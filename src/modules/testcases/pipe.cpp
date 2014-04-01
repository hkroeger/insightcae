/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "pipe.h"

#include "base/factory.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/openfoamcaseelements.h"

#include <boost/assign/list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>
#include "boost/ptr_container/ptr_container.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"

#include "gnuplot-iostream.h"

using namespace arma;
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{

CorrelationFunctionModel::CorrelationFunctionModel()
: B_(1.0), omega_(1.0)
{}

int CorrelationFunctionModel::numP() const
{
  return 2;
}

void CorrelationFunctionModel::setParameters(const double* params)
{
  B_=max(0.1, params[0]);
  omega_=params[1];
}

arma::mat CorrelationFunctionModel::weights(const arma::mat& x) const
{
  return exp( -x / max(x.col(0)) );
}

arma::mat CorrelationFunctionModel::evaluateObjective(const arma::mat& x) const
{
  //cout<<exp(-B_*x.col(0)) % cos(omega_*x.col(0))<<endl;
  return exp(-B_*x.col(0)) % ( cos(omega_*x.col(0)) );
}

double CorrelationFunctionModel::lengthScale() const
{
  return B_ / ( pow(B_, 2)+pow(omega_, 2) );
}

const char * RadialTPCArray::cmptNames[] = 
{ "xx", "xy", "xz",
  "yx", "yy", "yz",
  "zx", "zy", "zz" };
  
RadialTPCArray::RadialTPCArray(OpenFOAMCase& cm, Parameters const &p )
: OpenFOAMCaseElement(cm, p.name_prefix()+"RadialTPCArray"),
  p_(p)
{
  int n_r=10;
  for (int i=1; i<n_r; i++) // omit first and last
  {
    double x = double(i)/(n_r);
    double r = -cos(M_PI*(0.5+0.5*x))*p_.R();
    
    cout<<"Creating tpc FOs at r="<<r<<endl;
    
    r_.push_back(r);
    
    tpc_tan_.push_back(new cylindricalTwoPointCorrelation(cm, cylindricalTwoPointCorrelation::Parameters()
      .set_name(p_.name_prefix()+"_ax_"+lexical_cast<string>(i))
      .set_outputControl("timeStep")
      .set_p0(vec3(r, 0, p_.x()))
      .set_directionSpan(vec3(0, p_.tanSpan(), 0)) 
      .set_np(p_.np())
      .set_homogeneousTranslationUnit(vec3(0, 2.*M_PI/double(p_.nph()), 0))
      .set_nph( p_.nph() )
      .set_er(vec3(0, 1, 0))
      .set_ez(vec3(1, 0, 0))
      .set_degrees(false)
      .set_timeStart( p_.timeStart() )
    ));
    
    tpc_ax_.push_back(new cylindricalTwoPointCorrelation(cm, cylindricalTwoPointCorrelation::Parameters()
      .set_name(p_.name_prefix()+"_ax_"+lexical_cast<string>(i))
      .set_outputControl("timeStep")
      .set_p0(vec3(r, 0, p_.x()))
      .set_directionSpan(vec3(0, 0, p_.axSpan())) 
      .set_np(p_.np())
      .set_homogeneousTranslationUnit(vec3(0, 2.*M_PI/double(p_.nph()), 0))
      .set_nph(p_.nph())
      .set_er(vec3(0, 1, 0))
      .set_ez(vec3(1, 0, 0))
      .set_degrees(false)
      .set_timeStart( p_.timeStart() )
    ));
    
  }  
}

void RadialTPCArray::addIntoDictionaries(OFdicts& dictionaries) const
{
  BOOST_FOREACH(const cylindricalTwoPointCorrelation& tpc, tpc_tan_)
  {
    tpc.addIntoDictionaries(dictionaries);
  }
  BOOST_FOREACH(const cylindricalTwoPointCorrelation& tpc, tpc_ax_)
  {
    tpc.addIntoDictionaries(dictionaries);
  }
}

void RadialTPCArray::evaluate(OpenFOAMCase& cm, const boost::filesystem::path& location, ResultSetPtr& results) const
{
  evaluateSingle(cm, location, results, 
		  p_.name_prefix()+"_tan", 
		  p_.tanSpan(), "Angle [rad]",
		  tpc_tan_, 
		  "two-point correlation of velocity along tangential direction at different radii"
		);
  
  evaluateSingle(cm, location, results, 
		  p_.name_prefix()+"_ax", 
		  p_.axSpan(),  "Axial distance [length]",
		  tpc_ax_, 
		  "two-point correlation of velocity along axial direction at different radii"
		);
}

void RadialTPCArray::evaluateSingle
(
  OpenFOAMCase& cm, const boost::filesystem::path& location, 
  ResultSetPtr& results, 
  const std::string& name_prefix,
  double span,
  const std::string& axisLabel,
  const boost::ptr_vector<cylindricalTwoPointCorrelation>& tpcarray,
  const std::string& shortDescription
) const
{
  int nk=9;
  int nr=tpcarray.size();
  
  arma::mat L(r_.data(), r_.size(), 1);
  L=arma::join_rows(L, arma::zeros(r_.size(), nk)); // append nk column with 0's

  // create one plot per component, with the profiles for all radii overlayed
  {
    Gnuplot gp[nk];
    std::ostringstream cmd[nk];
    arma::mat data[nk], regressions[nk];
    
    for(int k=0; k<nk; k++)
    {
      std::string chart_name=name_prefix+"_"+cmptNames[k];
      std::string chart_file_name=chart_name+".png";
      
      gp[k]<<"set terminal png; set output '"<<chart_file_name<<"';";
      gp[k]<<"set xlabel '"<<axisLabel<<"'; set ylabel '<R_"<<cmptNames[k]<<">'; set grid; ";
      cmd[k]<<"plot 0 not lt -1";
      data[k]=zeros(p_.np(), nr+1);
      data[k].col(0)=arma::linspace<arma::mat>(0, span, p_.np());
      regressions[k]=zeros(p_.np(), nr+1);
      regressions[k].col(0)=arma::linspace<arma::mat>(0, span, p_.np());

      results->insert(chart_name,
	std::auto_ptr<Image>(new Image
	(
	chart_file_name, 
	shortDescription, ""
      )));

    }
    int ir=0;
    BOOST_FOREACH(const cylindricalTwoPointCorrelation& tpc, tpcarray)
    {
      boost::ptr_vector<arma::mat> res=twoPointCorrelation::readCorrelations(cm, location, tpc.name());
      
      // append profile of this radius to chart of this component
      for (int k=0; k<nk; k++)
      {
	cmd[k]<<", '-' w p lt "<<ir+1<<" t 'r="<<r_[ir]<<"', '-' w l lt "<<ir+1<<" t 'r="<<r_[ir]<<" (fit)'";
	data[k].col(ir+1) = res[k+1].row(res[k+1].n_rows-1).t();
	data[k].col(ir+1) /= data[k].col(ir+1)(0); // Normalize
	
	CorrelationFunctionModel m;
	cout<<"Fitting TPC for radius "<<ir<<" (r="<<r_[ir]<<"), component k="<<k<<" ("<<cmptNames[k]<<")"<<endl;
	nonlinearRegression(data[k].col(ir+1), data[k].col(0), m);
	regressions[k].col(ir+1)=m.evaluateObjective(regressions[k].col(0));
	
	L(ir, 1+k)=m.lengthScale();
      }
      ir++;
    }
      
    for (int k=0; k<nk; k++)
    {
      gp[k]<<cmd[k].str()<<endl;
      for (int c=1; c<data[k].n_cols; c++)
      {
	arma::mat pdata;
	pdata=join_rows(data[k].col(0), data[k].col(c));
	gp[k].send1d( pdata );
	pdata=join_rows(regressions[k].col(0), regressions[k].col(c));
	gp[k].send1d( pdata );
      }
    }
  }
  
  {
    std::string chart_name=name_prefix+"_L_diag";
    std::string chart_file_name=chart_name+".png";

    Gnuplot gp;
    std::ostringstream cmd;
    gp<<"set terminal png; set output '"<<chart_file_name<<"';";
    gp<<"set xlabel 'Radius [length]'; set ylabel 'L [length]'; set grid; ";
    cmd<<"plot 0 not lt -1";
    
    std::vector<double> ks=list_of<double>(0)(4)(8);
    
    BOOST_FOREACH(int k, ks)
    {
      cmd<<", '-' w lp lt "<<k+1<<" t 'L_"<<cmptNames[k]<<"'";
    }
    gp<<cmd.str()<<endl;
    BOOST_FOREACH(int k, ks)
    {
      arma::mat pdata;
      pdata=join_rows(L.col(0), L.col(k+1));
      gp.send1d(pdata);
    }
    
    results->insert(chart_name,
      std::auto_ptr<Image>(new Image
      (
      chart_file_name, 
      "Autocorrelation lengths", ""
    )));
  }

  {
    std::string chart_name=name_prefix+"_L_offdiag";
    std::string chart_file_name=chart_name+".png";

    Gnuplot gp;
    std::ostringstream cmd;
    gp<<"set terminal png; set output '"<<chart_file_name<<"';";
    gp<<"set xlabel 'Radius [length]'; set ylabel 'L [length]'; set grid; ";
    cmd<<"plot 0 not lt -1";
    
    std::vector<double> ks=list_of<double>(1)(2)(3)(5)(6)(7);
    
    BOOST_FOREACH(int k, ks)
    {
      cmd<<", '-' w lp lt "<<k+1<<" t 'L_"<<cmptNames[k]<<"'";
    }
    gp<<cmd.str()<<endl;
    BOOST_FOREACH(int k, ks)
    {
      arma::mat pdata;
      pdata=join_rows(L.col(0), L.col(k+1));
      gp.send1d(pdata);
    }
    
    results->insert(chart_name,
      std::auto_ptr<Image>(new Image
      (
      chart_file_name, 
      "Cross-correlation lengths", ""
    )));
  }

}
  
  
defineType(PipeBase);

PipeBase::PipeBase(const NoParameters&)
: OpenFOAMAnalysis
  (
    "Pipe Flow Test Case",
    "Cylindrical domain with cyclic BCs on axial ends"
  ),
  cycl_in_("cycl_half0"),
  cycl_out_("cycl_half1")
{}

PipeBase::~PipeBase()
{

}

ParameterSet PipeBase::defaultParameters() const
{
  ParameterSet p(OpenFOAMAnalysis::defaultParameters());
  
  p.extend
  (
    boost::assign::list_of<ParameterSet::SingleEntry>
    
      ("geometry", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("D",		new DoubleParameter(2.0, "[m] Diameter of the pipe"))
	    ("L",		new DoubleParameter(12.0, "[m] Length of the pipe"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Geometrical properties of the bearing"
	))
      
      ("mesh", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("nax",	new IntParameter(100, "# cells in axial direction"))
	    ("s",	new DoubleParameter(1.0, "Axial grid anisotropy (ratio of axial cell edge length to lateral edge length)"))
	    ("x",	new DoubleParameter(0.5, "Edge length of core block as fraction of diameter"))
	    ("ypluswall",	new DoubleParameter(0.5, "yPlus at the wall grid layer"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Properties of the computational mesh"
	))
      
      ("operation", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("Re_tau",		new DoubleParameter(180, "[-] Friction-Velocity-Reynolds number"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Definition of the operation point under consideration"
	))
      
      ("evaluation", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("inittime",	new DoubleParameter(10, "[T] length of grace period before averaging starts (as multiple of flow-through time)"))
	    ("meantime",	new DoubleParameter(10, "[T] length of time period for averaging of velocity and RMS (as multiple of flow-through time)"))
	    ("mean2time",	new DoubleParameter(10, "[T] length of time period for averaging of second order statistics (as multiple of flow-through time)"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Options for statistical evaluation"
	))
      
      .convert_to_container<ParameterSet::EntryList>()
  );
  
  return p;
}

std::string PipeBase::cyclPrefix() const
{
  boost:smatch m;
  boost::regex_search(cycl_in_, m, boost::regex("(.*)_half[0,1]"));
  std::string namePrefix=m[1];
  cout<<namePrefix<<endl;
  return namePrefix;
}


double PipeBase::calcLc(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "mesh", x);
  return x*D;
}

int PipeBase::calcnc(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);

  PSINT(p, "mesh", nax);
  PSDBL(p, "mesh", x);
  PSDBL(p, "mesh", s);
  double Delta=L/double(nax);
  return D*(M_PI+4.*x)/(8.*Delta/s);
}

int PipeBase::calcnr(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);

  PSINT(p, "mesh", nax);
  PSDBL(p, "mesh", s);
  PSDBL(p, "mesh", x);
  double Delta=L/double(nax);
  double lr=0.5*D*(1.-sqrt(2.)*x);
  int nr=max(1, bmd::GradingAnalyzer(calcgradr(p)).calc_n(calcywall(p), lr));
  cout<<"n_r="<<nr<<endl;
  return nr;
}

double PipeBase::calcywall(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "mesh", ypluswall);
  PSDBL(p, "operation", Re_tau);
  
  double ywall= ypluswall*0.5*D/Re_tau;
  cout<<"ywall = "<<ywall<<endl;
  return ywall;
}

double PipeBase::calcgradr(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);

  PSINT(p, "mesh", nax);
  PSDBL(p, "mesh", s);

  double Delta=L/double(nax);
  double delta0=calcywall(p);
  double grad=(Delta/s) / delta0;
  cout<<"Grading = "<<grad<<endl;
  return grad;
}

double PipeBase::calcUtau(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);

  return 1./(0.5*D);
}

#include <gsl/gsl_errno.h>
#include <gsl/gsl_roots.h>

double lambda_func(double lambda, void *param)
{
  double Retau=*static_cast<double*>(param);
  //cout<<Retau<<" "<<lambda<<endl;
  //cout<<(2./5.)+1./(2.*lambda)<<endl;
  double Re=pow(10, (2./5.)+1./(2.*lambda))/sqrt(lambda);
  return 2.*Retau*sqrt(8./lambda) - Re;
}

double PipeBase::calcRe(const ParameterSet& p) const
{
/*  
  PSDBL(p, "operation", Re_tau);
  int i, times, status;
  gsl_function f;
  gsl_root_fsolver *workspace_f;
  double x, x_l, x_r;

 
    workspace_f = gsl_root_fsolver_alloc(gsl_root_fsolver_bisection);
 
    f.function = &lambda_func;
    f.params = &Re_tau;
 
    x_l = 1e-2;
    x_r = 10;
 
    gsl_root_fsolver_set(workspace_f, &f, x_l, x_r);
 
    for(times = 0; times < 100; times++)
    {
        status = gsl_root_fsolver_iterate(workspace_f);
 
        x_l = gsl_root_fsolver_x_lower(workspace_f);
        x_r = gsl_root_fsolver_x_upper(workspace_f);
 
        status = gsl_root_test_interval(x_l, x_r, 1.0e-13, 1.0e-20);
        if(status != GSL_CONTINUE)
        {
            break;
        }
    }
 
    gsl_root_fsolver_free(workspace_f);
    double lambda=x_l;
    double Re=2.*Re_tau*sqrt(8./x_l);
    cout<<"Re="<<Re<<endl;
    return Re;*/
  PSDBL(p, "operation", D);
  PSDBL(p, "operation", Re_tau);
  double nu=1./Re_tau;
  return calcUbulk(p)*D/nu;
}

double PipeBase::calcUbulk(const ParameterSet& p) const
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "operation", Re_tau);
  double k=0.41;
  double Cplus=5.0;
  
  double nu=1./Re_tau;
  double rho=1.0;
  double tau0= pow(Re_tau*nu*sqrt(rho)/(0.5*D), 2);
  return sqrt(tau0/rho)*(1./k)*log(Re_tau)+Cplus; //calcRe(p)*(1./Re_tau)/D;
}

double PipeBase::calcT(const ParameterSet& p) const
{
  PSDBL(p, "geometry", L);
  return L/calcUbulk(p);
}

void PipeBase::createMesh
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  // create local variables from ParameterSet
  path dir = executionPath();
  
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);

  PSINT(p, "mesh", nax);
  
  double Lc=calcLc(p);
  int nc=calcnc(p);
  int nr=calcnr(p);
  double gradr=calcgradr(p);
  cout<<"Lc="<<Lc<<", nc="<<nc<<", nr="<<nr<<", grad_r="<<gradr<<endl;
    
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "wall");
  
  
  double al = M_PI/2.;
  
  std::map<int, Point> pts;
  pts = boost::assign::map_list_of   
      (11, 	vec3(0, 0.5*D, 0))
      (10, 	vec3(0,  cos(0.5*al)*Lc, 0.))
      (9, 	vec3(0,  1.2*0.5*Lc, 0.))
  ;
  arma::mat vL=vec3(L, 0, 0);
  arma::mat ax=vec3(1, 0, 0);
  
  // create patches
  Patch& cycl_in= 	bmd->addPatch(cycl_in_, new Patch());
  Patch& cycl_out= 	bmd->addPatch(cycl_out_, new Patch());
  
  {
    arma::mat r0=rotMatrix(0.5*al, ax);
    arma::mat r1=rotMatrix(1.5*al, ax);
    arma::mat r2=rotMatrix(2.5*al, ax);
    arma::mat r3=rotMatrix(3.5*al, ax);

    Block& bl = bmd->addBlock
    (  
      new Block(P_8(
	  r1*pts[10], r2*pts[10], r3*pts[10], r0*pts[10],
	  (r1*pts[10])+vL, (r2*pts[10])+vL, (r3*pts[10])+vL, (r0*pts[10])+vL
	),
	nc, nc, nax
      )
    );
    cycl_in.addFace(bl.face("0321"));
    cycl_out.addFace(bl.face("4567"));
  }

  for (int i=0; i<4; i++)
  {
    arma::mat r0=rotMatrix(double(i+0.5)*al, ax);
    arma::mat r1=rotMatrix(double(i+1.5)*al, ax);
    
    Block& bl = bmd->addBlock
    (
      new Block(P_8(
	  r1*pts[10], r0*pts[10], r0*pts[11], r1*pts[11],
	  (r1*pts[10])+vL, (r0*pts[10])+vL, (r0*pts[11])+vL, (r1*pts[11])+vL
	),
	nc, nr, nax,
	list_of<double>(1)(1./gradr)(1)
      )
    );
    cycl_in.addFace(bl.face("0321"));
    cycl_out.addFace(bl.face("4567"));

    arma::mat rmid=rotMatrix(double(i+1)*al, ax);
    bmd->addEdge(new ArcEdge(r1*pts[11], r0*pts[11], rmid*pts[11]));
    bmd->addEdge(new ArcEdge((r1*pts[11])+vL, (r0*pts[11])+vL, (rmid*pts[11])+vL));

    //inner core
    bmd->addEdge(new ArcEdge(r1*pts[10], r0*pts[10], rmid*pts[9]));
    bmd->addEdge(new ArcEdge((r1*pts[10])+vL, (r0*pts[10])+vL, (rmid*pts[9])+vL));

  }

  cm.insert(bmd.release());

  cm.createOnDisk(dir);
  cm.executeCommand(dir, "blockMesh");  
}


void PipeBase::createCase
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{
  // create local variables from ParameterSet
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  double T=calcT(p);
  cout << "Flow-through time T="<<T<<endl;
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

  
  cm.insert(new pimpleFoamNumerics(cm, pimpleFoamNumerics::Parameters().set_LES(true) ) );
  cm.insert(new cuttingPlane(cm, cuttingPlane::Parameters()
    .set_name("plane")
    .set_basePoint(vec3(0,0,0))
    .set_normal(vec3(0,0,1))
    .set_fields(list_of<string>("p")("U")("UMean")("UPrime2Mean"))
  ));
  
  cm.insert(new fieldAveraging(cm, fieldAveraging::Parameters()
    .set_name("averaging")
    .set_fields(list_of<std::string>("p")("U"))
    .set_timeStart(inittime*T)
  ));
  
  cm.insert(new RadialTPCArray(cm, RadialTPCArray::Parameters()
    .set_name_prefix("tpc_interior")
    .set_R(0.5*D)
    .set_x(0.5*L)
    .set_axSpan(0.5*L)
    .set_tanSpan(M_PI)
    .set_timeStart( (inittime+meantime)*T )
  ));
  
  /*
  cm.insert(new probes(cm, probes::Parameters()
    .set_name("probes")
    .set_fields(list_of<std::string>("p")("U"))
    .set_timeStart(inittime*T)
    .set_probeLocations(list_of<arma::mat>
      (vec3(0.1*L, 0, 0))
      (vec3(0.33*L, 0, 0))
      (vec3(0.5*L, 0, 0))
      (vec3(0.66*L, 0, 0))
      (vec3(0.9*L, 0, 0))
      (vec3(0.1*L, 0.9*0.5*D, 0))
      (vec3(0.33*L, 0.9*0.5*D, 0))
      (vec3(0.5*L, 0.9*0.5*D, 0))
      (vec3(0.66*L, 0.9*0.5*D, 0))
      (vec3(0.9*L, 0.9*0.5*D, 0))
    )
  ));
  */
  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(1./Re_tau) ));
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());
  insertTurbulenceModel(cm, p.get<SelectionParameter>("fluid/turbulenceModel").selection());

  cout<<"Ubulk="<<calcUbulk(p)<<endl;
}


  
ResultSetPtr PipeBase::evaluateResults(OpenFOAMCase& cm, const ParameterSet& p)
{
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  
  ResultSetPtr results = OpenFOAMAnalysis::evaluateResults(cm, p);
  
  boost::ptr_vector<sampleOps::set> sets;
  
  double x=L*0.5;
  sets.push_back(new sampleOps::circumferentialAveragedUniformLine(sampleOps::circumferentialAveragedUniformLine::Parameters()
    .set_name("radial")
    .set_start( vec3(x, 0,  0.01* 0.5*D))
    .set_end(   vec3(x, 0, 0.997* 0.5*D))
    .set_axis(vec3(1,0,0))
  ));
  
  sample(cm, executionPath(), 
     list_of<std::string>("p")("U")("UMean")("UPrime2Mean"),
     sets
  );
  
  sampleOps::ColumnDescription cd;
  arma::mat data = static_cast<sampleOps::circumferentialAveragedUniformLine&>(*sets.begin())
    .readSamples(cm, executionPath(), &cd);
    
    
  // Mean velocity profiles
  {
    Gnuplot gp;
    string chart_name="mean_velocity";
    string chart_file_name=chart_name+".png";
    
    gp<<"set terminal png; set output '"<<chart_file_name<<"';";
    gp<<"set xlabel 'y+'; set ylabel '<U+>'; set grid; ";
    gp<<"set logscale x;";
    
    int c=cd["UMean"].col;
    
    double fac_yp=Re_tau*2.0/D;
    double fac_Up=1./calcUtau(p);
    gp<<"plot 0 not lt -1,"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Up<<") w l t 'Axial',"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Up<<") w l t 'Circumferential',"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Up<<") w l t 'Radial'"<<endl;
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c)))   );
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+1))) );
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+2))) );

    results->insert(chart_name,
      std::auto_ptr<Image>(new Image
      (
      chart_file_name, 
      "Radial profiles of averaged velocities", ""
    )));
    
  }
  
  // Mean reynolds stress profiles
  {
    Gnuplot gp;
    string chart_name="mean_Rstress";
    string chart_file_name=chart_name+".png";
    double fac_yp=Re_tau*2.0/D;
    double fac_Rp=1./pow(calcUtau(p),2);
    int c=cd["UPrime2Mean"].col;
    
    gp<<"set terminal png; set output '"<<chart_file_name<<"';";
    gp<<"set xlabel 'y+'; set ylabel '<R+>'; set grid; ";
    gp<<"set logscale x;";
    gp<<"set yrange [:"<<fac_Rp*max(data.col(c))<<"];";
    
    
    gp<<"plot 0 not lt -1,"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Rp<<") w l t 'Rxx (Axial)',"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Rp<<") w l t 'Ryy (Circumferential)',"
	" '-' u (("<<Re_tau<<"-$1*"<<fac_yp<<")):($2*"<<fac_Rp<<") w l t 'Rzz (Radial)'"<<endl;
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c)))   );
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+3))) );
    gp.send1d( arma::mat(join_rows(data.col(0), data.col(c+5))) );

    results->insert(chart_name,
      std::auto_ptr<Image>(new Image
      (
      chart_file_name, 
      "Radial profiles of averaged reynolds stresses", ""
    )));
    
  }

  const RadialTPCArray* tpcs=cm.get<RadialTPCArray>("tpc_interiorRadialTPCArray");
  if (!tpcs)
    throw insight::Exception("tpc FO array not found in case!");
  tpcs->evaluate(cm, executionPath(), results);
  
  std::string init=
      "cbi=loadOFCase('.')\n"
      "prepareSnapshots()\n";
      
  runPvPython
  (
    cm, executionPath(), list_of<std::string>
    (
      init+
      "eb = planarSlice(cbi, [0,0,1e-6], [0,0,1])\n"
      "Show(eb)\n"
      "displayContour(eb, 'p', arrayType='CELL_DATA', barpos=[0.5,0.7], barorient=0)\n"
      "setCam([0,0,10], [0,0,0], [0,1,0])\n"
      "WriteImage('pressure_longi.jpg')\n"
    )
  );
  results->insert("pressureContour",
    std::auto_ptr<Image>(new Image
    (
    "pressure_longi.jpg", 
    "Contour of pressure (longitudinal section)", ""
  )));
  
  for(int i=0; i<3; i++)
  {
    std::string c("x"); c[0]+=i;
    runPvPython
    (
      cm, executionPath(), list_of<std::string>
      (
	init+
	"eb = planarSlice(cbi, [0,0,1e-6], [0,0,1])\n"
	"Show(eb)\n"
	"displayContour(eb, 'U', arrayType='CELL_DATA', component="+lexical_cast<char>(i)+", barpos=[0.5,0.7], barorient=0)\n"
	"setCam([0,0,10], [0,0,0], [0,1,0])\n"
	"WriteImage('U"+c+"_longi.jpg')\n"
      )
    );
    results->insert("U"+c+"Contour",
      std::auto_ptr<Image>(new Image
      (
      "U"+c+"_longi.jpg", 
      "Contour of "+c+"-Velocity", ""
    )));
  }

  
  return results;
}




defineType(PipeCyclic);

PipeCyclic::PipeCyclic(const NoParameters& nop)
: PipeBase(nop)
{
}

void PipeCyclic::createMesh
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  PipeBase::createMesh(cm, p);
  convertPatchPairToCyclic(cm, executionPath(), cyclPrefix());
}

void PipeCyclic::createCase
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  // create local variables from ParameterSet
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  double T=calcT(p);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);
      
  cm.insert(new CyclicPairBC(cm, cyclPrefix(), boundaryDict));
  
  PipeBase::createCase(cm, p);
  
  cm.insert(new PressureGradientSource(cm, PressureGradientSource::Parameters()
					    .set_Ubar(vec3(calcUbulk(p), 0, 0))
		));
/*  
  int n_r=10;
  for (int i=1; i<n_r; i++) // omit first and last
  {
    double x = double(i)/(n_r);
    double r = -cos(M_PI*(0.5+0.5*x))*0.5*D;
    cout<<"Inserting tpc FO at r="<<r<<endl;
    
    cm.insert(new cylindricalTwoPointCorrelation(cm, cylindricalTwoPointCorrelation::Parameters()
      .set_name("tpc_tan_"+lexical_cast<string>(i))
      .set_outputControl("timeStep")
      .set_p0(vec3(r, 0, 0.5*L))
      .set_directionSpan(vec3(0, M_PI, 0)) 
      .set_np(50)
      .set_homogeneousTranslationUnit(vec3(0, M_PI/2., 0))
      .set_nph(8)
      .set_er(vec3(0, 1, 0))
      .set_ez(vec3(1, 0, 0))
      .set_degrees(false)
      .set_timeStart( (inittime+meantime)*T )
    ));
    
    cm.insert(new cylindricalTwoPointCorrelation(cm, cylindricalTwoPointCorrelation::Parameters()
      .set_name("tpc_ax_"+lexical_cast<string>(i))
      .set_outputControl("timeStep")
      .set_p0(vec3(r, 0, 0))
      .set_directionSpan(vec3(0, 0, 0.5*L)) 
      .set_np(50)
      .set_homogeneousTranslationUnit(vec3(0, M_PI/2., 0))
      .set_nph(8)
      .set_er(vec3(0, 1, 0))
      .set_ez(vec3(1, 0, 0))
      .set_degrees(false)
      .set_timeStart( (inittime+meantime)*T )
    ));
    
  }*/
}

void PipeCyclic::applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p)
{
  PSDBL(p, "operation", Re_tau);
  
  /*
  setFields(cm, executionPath(), 
	    list_of<setFieldOps::FieldValueSpec>
	      ("volVectorFieldValue U ("+lexical_cast<string>(calcUbulk(p))+" 0 0)"),
	    ptr_vector<setFieldOps::setFieldOperator>()
  );
  cm.executeCommand(executionPath(), "applyBoundaryLayer", list_of<string>("-ybl")(lexical_cast<string>(0.25)) );
  cm.executeCommand(executionPath(), "randomizeVelocity", list_of<string>(lexical_cast<string>(0.1*calcUbulk(p))) );
  */
  
  cm.executeCommand(executionPath(), "perturbU", 
		    list_of<string>
		    (lexical_cast<string>(Re_tau))
		    ("("+lexical_cast<string>(calcUbulk(p))+" 0 0)") 
		   );
  
  OpenFOAMAnalysis::applyCustomPreprocessing(cm, p);
}

void PipeCyclic::applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts)
{
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  PSDBL(p, "evaluation", mean2time);
  double T=calcT(p);

  OpenFOAMAnalysis::applyCustomOptions(cm, p, dicts);
  
  OFDictData::dictFile& decomposeParDict=dicts->addDictionaryIfNonexistent("system/decomposeParDict");
  int np=decomposeParDict.getInt("numberOfSubdomains");
  OFDictData::dict msd;
  OFDictData::list dl;
  dl.push_back(np);
  dl.push_back(1);
  dl.push_back(1);
  msd["n"]=dl;
  msd["delta"]=0.001;
  decomposeParDict["method"]="simple";
  decomposeParDict["simpleCoeffs"]=msd;

  OFDictData::dictFile& controlDict=dicts->addDictionaryIfNonexistent("system/controlDict");
  if (cm.OFversion()<=160)
  {
    controlDict["application"]="channelFoam";
  }
  controlDict["endTime"] = (inittime+meantime+mean2time)*T;
}

addToFactoryTable(Analysis, PipeCyclic, NoParameters);




defineType(PipeInflow);

PipeInflow::PipeInflow(const NoParameters& nop)
: PipeBase(nop)
{
}

void PipeInflow::createMesh
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  PipeBase::createMesh(cm, p);
  //convertPatchPairToCyclic(cm, executionPath(), cyclPrefix());
}

void PipeInflow::createCase
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  // create local variables from ParameterSet
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  double T=calcT(p);

  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);
      
  cm.insert(new TurbulentVelocityInletBC(cm, cycl_in_, boundaryDict, TurbulentVelocityInletBC::Parameters()
    .set_velocity(vec3(calcUbulk(p), 0, 0))
    .set_turbulenceIntensity(0.05)
    .set_mixingLength(0.1*D)
  ));
  
  cm.insert(new PressureOutletBC(cm, cycl_out_, boundaryDict, PressureOutletBC::Parameters()
    .set_pressure(0.0)
  ));
  
  PipeBase::createCase(cm, p);
  
  cm.insert(new RadialTPCArray(cm, RadialTPCArray::Parameters()
    .set_name_prefix("tpc_inlet")
    .set_R(0.5*D)
    .set_x(0.)
    .set_axSpan(0.5*L)
    .set_tanSpan(M_PI)
    .set_timeStart( (inittime+meantime)*T )
  ));

//   int n_r=10;
//   for (int i=1; i<n_r; i++) // omit first and last
//   {
//     double x = double(i)/(n_r);
//     double r = -cos(M_PI*(0.5+0.5*x))*0.5*D;
//     cout<<"Inserting tpc FO at r="<<r<<endl;
//     
//     cm.insert(new cylindricalTwoPointCorrelation(cm, cylindricalTwoPointCorrelation::Parameters()
//       .set_name("tpc_tan_"+lexical_cast<string>(i))
//       .set_outputControl("timeStep")
//       .set_p0(vec3(r, 0, 0.5*L))
//       .set_directionSpan(vec3(0, M_PI, 0)) 
//       .set_np(50)
//       .set_homogeneousTranslationUnit(vec3(0, M_PI/2., 0))
//       .set_nph(8)
//       .set_er(vec3(0, 1, 0))
//       .set_ez(vec3(1, 0, 0))
//       .set_degrees(false)
//       .set_timeStart(inittime*T)
//     ));
//     
//     cm.insert(new cylindricalTwoPointCorrelation(cm, cylindricalTwoPointCorrelation::Parameters()
//       .set_name("tpc_ax_"+lexical_cast<string>(i))
//       .set_outputControl("timeStep")
//       .set_p0(vec3(r, 0, 0))
//       .set_directionSpan(vec3(0, 0, 0.5*L)) 
//       .set_np(50)
//       .set_homogeneousTranslationUnit(vec3(0, M_PI/2., 0))
//       .set_nph(8)
//       .set_er(vec3(0, 1, 0))
//       .set_ez(vec3(1, 0, 0))
//       .set_degrees(false)
//       .set_timeStart(inittime*T)
//     ));
//     
//   }
}

void PipeInflow::applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p)
{
  
  setFields(cm, executionPath(), 
	    list_of<setFieldOps::FieldValueSpec>
	      ("volVectorFieldValue U ("+lexical_cast<string>(calcUbulk(p))+" 0 0)"),
	    ptr_vector<setFieldOps::setFieldOperator>()
  );
  
  cm.get<TurbulentVelocityInletBC>(cycl_in_+"BC")->initInflowBC(executionPath());
  
  OpenFOAMAnalysis::applyCustomPreprocessing(cm, p);
}

void PipeInflow::applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts)
{
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  PSDBL(p, "evaluation", mean2time);
  double T=calcT(p);

  OpenFOAMAnalysis::applyCustomOptions(cm, p, dicts);
  
  OFDictData::dictFile& controlDict=dicts->addDictionaryIfNonexistent("system/controlDict");
  controlDict["endTime"] = (inittime+meantime+mean2time)*T;
}

addToFactoryTable(Analysis, PipeInflow, NoParameters);

}
