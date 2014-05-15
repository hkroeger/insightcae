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

#include "channel.h"

#include "base/factory.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/basiccaseelements.h"
#include "refdata.h"

#include <boost/assign/list_of.hpp>
#include <boost/assign.hpp>
#include "boost/ptr_container/ptr_container.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"
#include "boost/format.hpp"

#include "gnuplot-iostream.h"

using namespace arma;
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{


defineType(ChannelBase);

double ChannelBase::Re(double Re_tau)
{
  double k=0.41;
  double Cplus=5.0;
  
  return Re_tau*((1./k)*log(Re_tau)+Cplus-1.7);
}


double ChannelBase::Retau(double Re)
{
  struct Obj: public Objective1D
  {
    double Re;
    virtual double operator()(double x) const { return Re-ChannelBase::Re(x); }
  } obj;
  obj.Re=Re;
  return nonlinearSolve1D(obj, 1e-3*Re, Re);
}

double ChannelBase::UmaxByUbulk(double Retau)
{
  return 1 + 2.64 * Retau/ChannelBase::Re(Retau);
}


ChannelBase::ChannelBase(const NoParameters&)
: OpenFOAMAnalysis
  (
    "Channel Flow Test Case",
    "Cylindrical domain with cyclic BCs on axial ends"
  ),
  cycl_in_("cycl_half0"),
  cycl_out_("cycl_half1")
{
}

ChannelBase::~ChannelBase()
{

}

ParameterSet ChannelBase::defaultParameters() const
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
	    ("H",		new DoubleParameter(2.0, "[m] Height of the channel"))
	    ("B",		new DoubleParameter(4.19, "[m] Width of the channel"))
	    ("L",		new DoubleParameter(12.56, "[m] Length of the channel"))
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
	    ("nh",	new IntParameter(100, "# cells in vertical direction"))
	    ("ypluswall",	new DoubleParameter(0.5, "yPlus at the wall grid layer"))
	    ("2d",	new BoolParameter(false, "Whether to create a two-dimensional case"))
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
      
      ("run", new SubsetParameter	
	    (
		  ParameterSet
		  (
		    boost::assign::list_of<ParameterSet::SingleEntry>
		    ("perturbU", 	new BoolParameter(true, "Whether to impose artifical perturbations on the initial velocity field"))
		    .convert_to_container<ParameterSet::EntryList>()
		  ), 
		  "Execution parameters"
      ))

      ("evaluation", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("inittime",	new DoubleParameter(5, "[T] length of grace period before averaging starts (as multiple of flow-through time)"))
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

std::string ChannelBase::cyclPrefix() const
{
  boost:smatch m;
  boost::regex_search(cycl_in_, m, boost::regex("(.*)_half[0,1]"));
  std::string namePrefix=m[1];
  return namePrefix;
}

void ChannelBase::calcDerivedInputData(const ParameterSet& p)
{
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);

  PSDBL(p, "mesh", ypluswall);
  PSINT(p, "mesh", nax);
  PSINT(p, "mesh", nh);
  
  // Physics
  Re_=Re(Re_tau);
  Ubulk_=Re_/Re_tau;
  T_=L/Ubulk_;
  nu_=1./Re_tau;
  utau_=Re_tau*nu_/(0.5*H);
  ywall_ = ypluswall/Re_tau;
  
  // grid
  double Delta=L/double(nax);
  
  if (p.getBool("mesh/2d"))
    nb_=1;
  else
    nb_=B/Delta;
  
  nh_=nh/2;
  gradh_=bmd::GradingAnalyzer(ywall_, H/2., nh_).grad();
  //nh_=max(1, bmd::GradingAnalyzer(gradh_).calc_n(ywall_, H/2.));
  
  cout<<"Derived data:"<<endl
      <<"============================================="<<endl;
  cout<<"Reynolds number \tRe="<<Re_<<endl;
  cout<<"Bulk velocity \tUbulk="<<Ubulk_<<endl;
  cout<<"Flow-through time \tT="<<T_<<endl;
  cout<<"Viscosity \tnu="<<nu_<<endl;
  cout<<"Friction velocity \tutau="<<utau_<<endl;
  cout<<"Wall distance of first grid point \tywall="<<ywall_<<endl;
  cout<<"# cells spanwise \tnb="<<nb_<<endl;
  cout<<"# grading vertical \tgradh="<<gradh_<<endl;
  cout<<"============================================="<<endl;
}

void ChannelBase::createMesh
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  // create local variables from ParameterSet
  path dir = executionPath();
  
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", L);

  PSINT(p, "mesh", nax);
      
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "wall");
  
  
  double al = M_PI/2.;
  
  std::map<int, Point> pts;
  pts = boost::assign::map_list_of   
      (0, 	vec3(0.5*L, -0.5*H, -0.5*B))
      (1, 	vec3(-0.5*L, -0.5*H, -0.5*B))
      (2, 	vec3(-0.5*L, -0.5*H, 0.5*B))
      (3, 	vec3(0.5*L, -0.5*H, 0.5*B))
  ;
  arma::mat vH=vec3(0, H, 0);
  
  // create patches
  Patch& cycl_in= 	bmd->addPatch(cycl_in_, new Patch());
  Patch& cycl_out= 	bmd->addPatch(cycl_out_, new Patch());
  Patch cycl_side_0=Patch();
  Patch cycl_side_1=Patch();
  
  string side_type="cyclic";
  if (p.getBool("mesh/2d")) side_type="empty";
  Patch& cycl_side= 	bmd->addPatch("cycl_side", new Patch(side_type));
  
  {
    Block& bl = bmd->addBlock
    (  
      new Block(P_8(
	  pts[0], pts[1], pts[2], pts[3],
	  (pts[0])+0.5*vH, (pts[1])+0.5*vH, (pts[2])+0.5*vH, (pts[3])+0.5*vH
	),
	nax, nb_, nh_,
	list_of<double>(1.)(1.)(gradh_)
      )
    );
    cycl_out.addFace(bl.face("0473"));
    cycl_in.addFace(bl.face("1265"));
    cycl_side_0.addFace(bl.face("0154"));
    cycl_side_1.addFace(bl.face("2376"));
  }

  {
    Block& bl = bmd->addBlock
    (  
      new Block(P_8(
	  (pts[0])+0.5*vH, (pts[1])+0.5*vH, (pts[2])+0.5*vH, (pts[3])+0.5*vH,
	  (pts[0])+1*vH, (pts[1])+1*vH, (pts[2])+1*vH, (pts[3])+1*vH
	),
	nax, nb_, nh_,
	list_of<double>(1.)(1.)(1./gradh_)
      )
    );
    cycl_out.addFace(bl.face("0473"));
    cycl_in.addFace(bl.face("1265"));
    cycl_side_0.addFace(bl.face("0154"));
    cycl_side_1.addFace(bl.face("2376"));
  }

  cycl_side.appendPatch(cycl_side_0);
  cycl_side.appendPatch(cycl_side_1);
  
  cm.insert(bmd.release());

  cm.createOnDisk(dir);
  cm.executeCommand(dir, "blockMesh");  
}


void ChannelBase::createCase
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{
  // create local variables from ParameterSet
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  PSDBL(p, "evaluation", mean2time);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

  cm.insert( new pimpleFoamNumerics(cm, pimpleFoamNumerics::Parameters()
    .set_maxDeltaT(0.25*T_)
    .set_writeControl("adjustableRunTime")
    .set_writeInterval(0.25*T_)
    .set_endTime( (inittime+meantime+mean2time)*T_ )
    .set_writeFormat("ascii")
    .set_decompositionMethod("simple")
    .set_deltaT(1e-3)
  ) );
  cm.insert(new extendedForces(cm, extendedForces::Parameters()
    .set_patches( list_of<string>("walls") )
  ));
  cm.insert(new fieldAveraging(cm, fieldAveraging::Parameters()
    .set_name("averaging")
    .set_fields(list_of<std::string>("p")("U")("pressureForce")("viscousForce"))
    .set_timeStart(inittime*T_)
  ));
  
  cm.insert(new LinearTPCArray(cm, LinearTPCArray::Parameters()
    .set_name_prefix("tpc_interior")
    .set_R(0.5*H)
    .set_x(0.0) // middle x==0!
    .set_z(-0.49*B)
    .set_axSpan(0.5*L)
    .set_tanSpan(0.45*B)
    .set_timeStart( (inittime+meantime)*T_ )
  ));
  

  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(nu_) ));
  if (p.getBool("mesh/2d"))
    cm.insert(new SimpleBC(cm, "cycl_side", boundaryDict, "empty") );
  else
    cm.insert(new CyclicPairBC(cm, "cycl_side", boundaryDict) );
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());
  
  insertTurbulenceModel(cm, p.get<SelectionParameter>("fluid/turbulenceModel").selection());

}

void ChannelBase::applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts)
{
  OpenFOAMAnalysis::applyCustomOptions(cm, p, dicts);
  
//   OFDictData::dictFile& controlDict=dicts->addDictionaryIfNonexistent("system/controlDict");
//   controlDict["maxDeltaT"]=0.5*T_;
}

void ChannelBase::evaluateAtSection(
  OpenFOAMCase& cm, const ParameterSet& p, 
  ResultSetPtr results, double x, int i
)
{
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  
  double xByL=x/L + 0.5;
  string title="section__xByL_" + str(format("%07.3f") % xByL);
  replace_all(title, ".", "_");
    
  boost::ptr_vector<sampleOps::set> sets;
  
  double delta_yp1=1./Re_tau;
  sets.push_back(new sampleOps::linearAveragedUniformLine(sampleOps::linearAveragedUniformLine::Parameters()
    .set_name("radial")
    .set_start( vec3(x, delta_yp1, -0.49*B))
    .set_end(   vec3(x, 0.5*H-delta_yp1, -0.49*B))
    .set_dir1(vec3(1,0,0))
    .set_dir2(vec3(0,0,0.98*B))
    .set_nd1(1)
    .set_nd2(5)
  ));
  
  sample(cm, executionPath(), 
     list_of<std::string>("p")("U")("UMean")("UPrime2Mean")("k")("omega")("epsilon")("nut"),
     sets
  );
  
  sampleOps::ColumnDescription cd;
  arma::mat data = static_cast<sampleOps::linearAveragedUniformLine&>(*sets.begin())
    .readSamples(cm, executionPath(), &cd);
      
  arma::mat refdata_umean180=refdatalib.getProfile("MKM_Channel", "180/umean_vs_yp");
  arma::mat refdata_wmean180=refdatalib.getProfile("MKM_Channel", "180/wmean_vs_yp");
  arma::mat refdata_umean590=refdatalib.getProfile("MKM_Channel", "590/umean_vs_yp");
  arma::mat refdata_wmean590=refdatalib.getProfile("MKM_Channel", "590/wmean_vs_yp");

  // Mean velocity profiles
  {
    Gnuplot gp;
    string chart_name="chartMeanVelocity_"+title;
    string chart_file_name=chart_name+".png";
    
    gp<<"set terminal pngcairo; set termoption dash; set output '"<<chart_file_name<<"';";
    gp<<"set xlabel 'y+'; set ylabel '<U+>'; set grid; ";
    gp<<"set logscale x;";
    
    int c=cd["UMean"].col;
    
    arma::mat axial(join_rows(Re_tau-Re_tau*data.col(0), data.col(c)));
    arma::mat spanwise(join_rows(Re_tau-Re_tau*data.col(0), data.col(c+1)));
    arma::mat wallnormal(join_rows(Re_tau-Re_tau*data.col(0), data.col(c+2)));
    
    axial.save( (executionPath()/("umeanaxial_vs_yp_"+title+".txt")).c_str(), arma_ascii);
    spanwise.save( (executionPath()/("umeanspanwise_vs_yp_"+title+".txt")).c_str(), arma_ascii);
    wallnormal.save( (executionPath()/("umeanwallnormal_vs_yp_"+title+".txt")).c_str(), arma_ascii);
    
    gp<<"plot 0 not lt -1,"
	" '-' w l lt 1 lc 1 lw 4 t 'Axial',"
	" '-' w l lt 1 lc 2 lw 4 t 'Spanwise',"
	" '-' w l lt 1 lc 3 lw 4 t 'Wall normal',"
	
	" '-' w l lt 2 lc 1 t 'Axial (MKM Re_tau=180)',"
	" '-' w l lt 2 lc 2 t 'Spanwise (MKM Re_tau=180)',"
	" '-' w l lt 3 lc 1 t 'Axial (MKM Re_tau=590)',"
	" '-' w l lt 3 lc 2 t 'Spanwise (MKM Re_tau=590)'"
	<<endl;
	
    gp.send1d( axial );
    gp.send1d( spanwise );
    gp.send1d( wallnormal );
    gp.send1d( refdata_umean180 );
    gp.send1d( refdata_wmean180 );
    gp.send1d( refdata_umean590 );
    gp.send1d( refdata_wmean590 );

    results->insert(chart_name,
      std::auto_ptr<Image>(new Image
      (
      chart_file_name, 
      "Wall normal profiles of averaged velocities at x/L=" + str(format("%g")%xByL), ""
    )));
    
  }
  
  // L profiles from k/omega
  if ((cd.find("k")!=cd.end()) && (cd.find("omega")!=cd.end()))
  {
    Gnuplot gp;
    string chart_name="chartTurbulentLengthScale_"+title;
    string chart_file_name=chart_name+".png";
    
    gp<<"set terminal png; set output '"<<chart_file_name<<"';";
    gp<<"set xlabel 'y_delta'; set ylabel '<L_delta_RANS>'; set grid; ";
    
    arma::mat k=data.col(cd["k"].col);
    arma::mat omega=data.col(cd["omega"].col);
    
    arma::mat ydelta=1.0-(data.col(0)+delta_yp1)/(0.5*H); //Re_tau-Re_tau*data.col(0);
    arma::mat Lt=(2./H)*sqrt(k)/(0.09*omega);
    arma::mat Ltp(join_rows(ydelta, Lt));
    Ltp.save( (executionPath()/("LdeltaRANS_vs_yp_"+title+".txt")).c_str(), arma_ascii);
    
    struct cfm : public RegressionModel
    {
      double c0, c1, c2, c3;
        virtual int numP() const { return 4; }
	virtual void setParameters(const double* params)
	{
	  c0=params[0];
	  c1=params[1];
	  c2=params[2];
	  c3=params[3];
	}
	virtual void setInitialValues(double* params) const
	{
	  params[0]=1.0;
	  params[1]=-1.0;
	  params[2]=0.1;
	  params[3]=0.1;
	}

	virtual arma::mat evaluateObjective(const arma::mat& x) const
	{
	  return c0*pow(x, c2) + c1*pow(x, c3);
	}
    } m;
    nonlinearRegression(Lt, ydelta, m);
    arma::mat yfit=m.evaluateObjective(ydelta);
    
    gp<<"plot 0 not lt -1,"
	" '-' w l t 'CFD', '-' w l t 'Fit "<<m.c0<<"*ydelta^"<<m.c2<<" + ("<<m.c1<<"*ydelta^"<<m.c3<<")'"
	<<endl;
    gp.send1d( Ltp );
    gp.send1d( arma::mat(join_rows(ydelta, yfit)) );

    results->insert
    (
     "regressionCoefficientsTubulentLengthScale_"+title,
     std::auto_ptr<AttributeTableResult>
     (
       new AttributeTableResult
       (
	 list_of<string>
	  ("c0")
	  ("c1")
	  ("c2")
	  ("c3"),

	 list_of<AttributeTableResult::AttributeValue>
	  (m.c0)(m.c1)(m.c2)(m.c3),
	"Regression coefficients", "", ""
	)
     )
    );
     
    results->insert(chart_name,
      std::auto_ptr<Image>(new Image
      (
      chart_file_name, 
      "Wall normal profile of turbulent length scale at x/L=" + str(format("%g")%xByL), 
      "The length scale is computed from the RANS model's k and omega field."
    )));
    
  }

  arma::mat refdata_Ruu=refdatalib.getProfile("MKM_Channel", "180/Ruu_vs_yp");
  arma::mat refdata_Rvv=refdatalib.getProfile("MKM_Channel", "180/Rvv_vs_yp");
  arma::mat refdata_Rww=refdatalib.getProfile("MKM_Channel", "180/Rww_vs_yp");
  arma::mat refdata_Ruu590=refdatalib.getProfile("MKM_Channel", "590/Ruu_vs_yp");
  arma::mat refdata_Rvv590=refdatalib.getProfile("MKM_Channel", "590/Rvv_vs_yp");
  arma::mat refdata_Rww590=refdatalib.getProfile("MKM_Channel", "590/Rww_vs_yp");
  
  arma::mat refdata_K=refdata_Ruu;
  refdata_K.col(1)+=Interpolator(refdata_Rvv)(refdata_Ruu.col(0));
  refdata_K.col(1)+=Interpolator(refdata_Rww)(refdata_Ruu.col(0));
  refdata_K.col(1)*=0.5;
  refdata_K.col(0)/=180.0;
  
  arma::mat refdata_K590=refdata_Ruu590;
  refdata_K590.col(1)+=Interpolator(refdata_Rvv590)(refdata_Ruu590.col(0));
  refdata_K590.col(1)+=Interpolator(refdata_Rww590)(refdata_Ruu590.col(0));
  refdata_K590.col(1)*=0.5;
  refdata_K590.col(0)/=590.0;
  

  // Mean reynolds stress profiles
  {
    string chart_name="chartMeanReyStress_"+title;
    string chart_file_name=chart_name+".png";
    
    int c=cd["UPrime2Mean"].col;
    arma::mat axial(join_rows(Re_tau-Re_tau*data.col(0), data.col(c)));
    arma::mat spanwise(join_rows(Re_tau-Re_tau*data.col(0), data.col(c+3)));
    arma::mat wallnormal(join_rows(Re_tau-Re_tau*data.col(0), data.col(c+5)));
    
    {    
      Gnuplot gp;
      gp<<"set terminal pngcairo; set termoption dash; set output '"<<chart_file_name<<"';";
      gp<<"set xlabel 'y+'; set ylabel '<R+>'; set grid; ";
      gp<<"set logscale x;";
      gp<<"set yrange [:"<<max(axial.col(1))<<"];";
      
      gp<<"plot 0 not lt -1,"
	  " '-' w l lt 1 lc 1 lw 4 t 'Rxx (Axial)',"
	  " '-' w l lt 1 lc 2 lw 4 t 'Ryy (Spanwise)',"
	  " '-' w l lt 1 lc 3 lw 4 t 'Rzz (Wall normal)',"
	  " '-' w l lt 2 lc 1 t 'Rxx (MKM Re_tau=180)',"
	  " '-' w l lt 2 lc 2 t 'Ryy (MKM Re_tau=180)',"
	  " '-' w l lt 2 lc 3 t 'Rzz (MKM Re_tau=180)',"
	  " '-' w l lt 3 lc 1 t 'Rxx (MKM Re_tau=590)',"
	  " '-' w l lt 3 lc 2 t 'Ryy (MKM Re_tau=590)',"
	  " '-' w l lt 3 lc 3 t 'Rzz (MKM Re_tau=590)'"
	  <<endl;
      gp.send1d( axial );
      gp.send1d( spanwise );
      gp.send1d( wallnormal );
      gp.send1d( refdata_Ruu );
      gp.send1d( refdata_Rvv );
      gp.send1d( refdata_Rww );
      gp.send1d( refdata_Ruu590 );
      gp.send1d( refdata_Rvv590 );
      gp.send1d( refdata_Rww590 );
    }
    
    results->insert(chart_name,
      std::auto_ptr<Image>(new Image
      (
      chart_file_name, 
      "Wall normal profiles of averaged reynolds stresses at x/L=" + str(format("%g")%xByL), ""
    )));

    chart_name="chartMeanTKE_"+title;
    chart_file_name=chart_name+".png";
    
    int ck=cd["k"].col;
    
    arma::mat K= 0.5*(data.col(c)+data.col(c+1)+data.col(c+2));
    if (cd.find("k")!=cd.end())
    {
      K+=data.col(ck);
    }
    else
      cout<<"not adding k"<<endl;
	  
    K /= pow(utau_, 2); // K => K+
    
    arma::mat Kp(join_rows( (1.-data.col(0)/(0.5*H)), K));
    Kp.save( (executionPath()/("Kp_vs_ydelta_"+title+".txt")).c_str(), arma_ascii);
    
    {
      Gnuplot gp;
      gp<<"set terminal png; set output '"<<chart_file_name<<"';";
      gp<<"set xlabel 'y_delta'; set ylabel '<K+>'; set grid; ";
      
      gp<<"plot 0 not lt -1,"
	  " '-' w l t 'TKE'"
	  ", '-' u 1:2 w l t 'DNS (Re_tau=180, MKM)'"
	  ", '-' u 1:2 w l t 'DNS (Re_tau=590, MKM)'"
	  <<endl;
      gp.send1d( Kp );
      gp.send1d( refdata_K );
      gp.send1d( refdata_K590 );
    }
    
    results->insert(chart_name,
      std::auto_ptr<Image>(new Image
      (
      chart_file_name, 
      "Wall normal profiles of averaged turbulent kinetic energy (1/2 R_ii + k_model) at x/L=" + str(format("%g")%xByL), ""
    )));
  }

  std::string init=
      "cbi=loadOFCase('.')\n"
      "prepareSnapshots()\n";
      
  std::string pressure_contour_name="contourPressure_ax_"+title;
  std::string pressure_contour_filename=pressure_contour_name+".png";
  runPvPython
  (
    cm, executionPath(), list_of<std::string>
    (
      init+
      "eb = planarSlice(cbi, ["+lexical_cast<string>(x)+",0,1e-6], [1,0,0])\n"
      "Show(eb)\n"
      "displayContour(eb, 'p', arrayType='CELL_DATA', barpos=[0.5,0.7], barorient=0)\n"
      "setCam([-10,0,0], [0,0,0], [0,1,0])\n"
      "WriteImage('"+pressure_contour_filename+"')\n"
    )
  );
  results->insert(pressure_contour_name,
    std::auto_ptr<Image>(new Image
    (
    pressure_contour_filename, 
    "Contour of pressure (axial section at x/L=" + str(format("%g")%xByL)+")", ""
  )));
  
  for(int i=0; i<3; i++)
  {
    std::string c("x"); c[0]+=i;
    std::string velocity_contour_name="contourU"+c+"_ax_"+title;
    string velocity_contour_filename=velocity_contour_name+".png";
    runPvPython
    (
      cm, executionPath(), list_of<std::string>
      (
	init+
	"eb = planarSlice(cbi, ["+lexical_cast<string>(x)+",0,1e-6], [1,0,0])\n"
	"Show(eb)\n"
	"displayContour(eb, 'U', arrayType='CELL_DATA', component="+lexical_cast<char>(i)+", barpos=[0.5,0.7], barorient=0)\n"
	"setCam([-10,0,0], [0,0,0], [0,1,0])\n"
	"WriteImage('"+velocity_contour_filename+"')\n"
      )
    );
    results->insert(velocity_contour_name,
      std::auto_ptr<Image>(new Image
      (
      velocity_contour_filename, 
      "Contour of "+c+"-Velocity (axial section at x/L=" + str(format("%g")%xByL)+")", ""
    )));
  }
}

ResultSetPtr ChannelBase::evaluateResults(OpenFOAMCase& cm, const ParameterSet& p)
{
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "mesh", nax);
  
  ResultSetPtr results = OpenFOAMAnalysis::evaluateResults(cm, p);
  
  evaluateAtSection(cm, p, results, 0.0, 0);

  const LinearTPCArray* tpcs=cm.get<LinearTPCArray>("tpc_interiorTPCArray");
  if (!tpcs)
    throw insight::Exception("tpc FO array not found in case!");
  tpcs->evaluate(cm, executionPath(), results, 
		 "two-point correlation of velocity at different radii at x/L=0.5"
  );
 
  // Wall friction coefficient
  arma::mat wallforce=viscousForceProfile(cm, executionPath(), vec3(1,0,0), nax);

  {
    Gnuplot gp;
    string chart_name="chartMeanWallFriction";
    string chart_file_name=chart_name+".png";
    
    gp<<"set terminal png; set output '"<<chart_file_name<<"';";
    gp<<"set xlabel 'x+'; set ylabel '<Cf>'; set grid; ";
    //gp<<"set logscale x;";
    
    arma::mat Cf_vs_xp(join_rows(
      wallforce.col(0)*Re_tau, 
      wallforce.col(1)/(0.5*pow(Ubulk_,2))
    ));
    Cf_vs_xp.save( (executionPath()/"Cf_vs_xp.txt").c_str(), arma_ascii);
    
    arma::mat Cftheo_vs_xp=zeros(2,2);
    Cftheo_vs_xp(0,0)=Cf_vs_xp(0,0);
    Cftheo_vs_xp(1,0)=Cf_vs_xp(Cf_vs_xp.n_rows-1,0);
    double Cftheo=pow(utau_,2)/(0.5*pow(Ubulk_,2));
    Cftheo_vs_xp(0,1)=Cftheo;
    Cftheo_vs_xp(1,1)=Cftheo;

    gp<<"plot 0 not lt -1,"
	" '-' w l t 'CFD'"
	", '-' w l t 'Analytical'"
	<<endl;
    gp.send1d( Cf_vs_xp );
    gp.send1d( Cftheo_vs_xp );

    results->insert(chart_name,
      std::auto_ptr<Image>(new Image
      (
      chart_file_name, 
      "Axial profile of wall shear stress", ""
    )));
    
  }

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
  results->insert("contourPressure",
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
    results->insert("contourU"+c,
      std::auto_ptr<Image>(new Image
      (
      "U"+c+"_longi.jpg", 
      "Contour of "+c+"-Velocity", ""
    )));
  }
  
  return results;
}




defineType(ChannelCyclic);

ChannelCyclic::ChannelCyclic(const NoParameters& nop)
: ChannelBase(nop)
{
}

ParameterSet ChannelCyclic::defaultParameters() const
{
  ParameterSet p(ChannelBase::defaultParameters());
  
  p.extend
  (
    boost::assign::list_of<ParameterSet::SingleEntry>
          
      ("run", new SubsetParameter	
	    (
		  ParameterSet
		  (
		    boost::assign::list_of<ParameterSet::SingleEntry>
		    ("perturbU", 	new BoolParameter(true, "Whether to impose artifical perturbations on the initial velocity field"))
		    .convert_to_container<ParameterSet::EntryList>()
		  ), 
		  "Execution parameters"
      ))
      
      .convert_to_container<ParameterSet::EntryList>()
  );
  
  return p;
}


void ChannelCyclic::createMesh
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  ChannelBase::createMesh(cm, p);
  convertPatchPairToCyclic(cm, executionPath(), cyclPrefix());
}

void ChannelCyclic::createCase
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  // create local variables from ParameterSet
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);
      
  cm.insert(new CyclicPairBC(cm, cyclPrefix(), boundaryDict));
  
  ChannelBase::createCase(cm, p);
  
  cm.insert(new PressureGradientSource(cm, PressureGradientSource::Parameters()
					    .set_Ubar(vec3(Ubulk_, 0, 0))
		));

}

void ChannelCyclic::applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p)
{
  if (p.getBool("run/perturbU"))
  {
    PSDBL(p, "operation", Re_tau);
    /*
    setFields(cm, executionPath(), 
	      list_of<setFieldOps::FieldValueSpec>
		("volVectorFieldValue U ("+lexical_cast<string>(Ubulk_)+" 0 0)"),
	      ptr_vector<setFieldOps::setFieldOperator>()
    );
    cm.executeCommand(executionPath(), "applyBoundaryLayer", list_of<string>("-ybl")(lexical_cast<string>(0.25)) );
    cm.executeCommand(executionPath(), "randomizeVelocity", list_of<string>(lexical_cast<string>(0.1*Ubulk_)) );
    */
    cm.executeCommand(executionPath(), "perturbU", 
		      list_of<string>
		      (lexical_cast<string>(Re_tau))
		      ("("+lexical_cast<string>(Ubulk_)+" 0 0)") 
		    );
  }
  OpenFOAMAnalysis::applyCustomPreprocessing(cm, p);
}

void ChannelCyclic::applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts)
{
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  PSDBL(p, "evaluation", mean2time);

  ChannelBase::applyCustomOptions(cm, p, dicts);
  
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
  controlDict["endTime"] = (inittime+meantime+mean2time)*T_;
}

addToFactoryTable(Analysis, ChannelCyclic, NoParameters);







defineType(ChannelInflow);

const char* ChannelInflow::tpc_names_[] = 
  {
    "tpc0_inlet",
    "tpc1_intermediate1",
    "tpc2_intermediate2",
    "tpc3_intermediate3"
  };

const double ChannelInflow::tpc_xlocs_[] = {0.0, 0.125, 0.25, 0.375};

ChannelInflow::ChannelInflow(const NoParameters& nop)
: ChannelBase(nop)
{
}

ParameterSet ChannelInflow::defaultParameters() const
{
  ParameterSet p(ChannelBase::defaultParameters());
  p.extend(TurbulentVelocityInletBC::inflowInitializer::defaultParameters().entries()); 
  return p;
}

void ChannelInflow::createMesh
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  ChannelBase::createMesh(cm, p);
  //convertPatchPairToCyclic(cm, executionPath(), cyclPrefix());
}

void ChannelInflow::createCase
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  // create local variables from ParameterSet
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", turbulenceModel);
  
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  PSDBL(p, "evaluation", mean2time);

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(executionPath(), boundaryDict);
      
  cm.insert(new TurbulentVelocityInletBC(cm, cycl_in_, boundaryDict, TurbulentVelocityInletBC::Parameters()
    .set_velocity(vec3(Ubulk_, 0, 0))
    .set_turbulenceIntensity(0.05)
    //.set_mixingLength(0.1*D)
    .set_initializer(TurbulentVelocityInletBC::channelInflowInitializer::Ptr(new TurbulentVelocityInletBC::channelInflowInitializer()))
  ));
  
  cm.insert(new PressureOutletBC(cm, cycl_out_, boundaryDict, PressureOutletBC::Parameters()
    .set_pressure(0.0)
  ));
  
  ChannelBase::createCase(cm, p);
  
  for (int i=0; i<ntpc_; i++)
  {
    cm.insert(new LinearTPCArray(cm, LinearTPCArray::Parameters()
      .set_name_prefix(tpc_names_[i])
      .set_R(0.5*H)
      .set_x((-0.5+tpc_xlocs_[i])*L)
      .set_z(-0.49*B)
      .set_axSpan(0.5*L)
      .set_tanSpan(0.45*B)
      .set_timeStart( (inittime+meantime)*T_ )
    ));
  }
  
}

ResultSetPtr ChannelInflow::evaluateResults(OpenFOAMCase& cm, const ParameterSet& p)
{
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  
  ResultSetPtr results = ChannelBase::evaluateResults(cm, p);
  
  // ============= Longitudinal profile of Velocity an RMS ================
  int nr=10;
  for (int i=0; i<nr; i++)
  {
    double r0=0.1, r1=0.997;
    double r=r0+(r1-r0)*double(i)/double(nr-1);
    double yByH=r/H;
    
    string title="longitudinal__yByH_"+str(format("%07.3f")%yByH);
    replace_all(title, ".", "_");

    boost::ptr_vector<sampleOps::set> sets;
    
    double delta_yp1=1./Re_tau;
    sets.push_back(new sampleOps::linearAveragedUniformLine(sampleOps::linearAveragedUniformLine::Parameters()
      .set_name("longitudinal"+lexical_cast<string>(i))
      .set_start( vec3(-0.5*L, r*0.5*H, -0.49*B))
      .set_end(   vec3(0.5*L, r*0.5*H, -0.49*B))
      .set_dir1(vec3(1,0,0))
      .set_dir2(vec3(0,0,0.98*B))
      .set_nd1(1)
      .set_nd2(5)
    ));
    
    sample(cm, executionPath(), 
      list_of<std::string>("p")("U")("UMean")("UPrime2Mean"),
      sets
    );
    
    sampleOps::ColumnDescription cd;
    arma::mat data = static_cast<sampleOps::linearAveragedUniformLine&>(*sets.begin())
      .readSamples(cm, executionPath(), &cd);
      
      
    // Mean velocity profiles
    {
      int c=cd["UMean"].col;
      
      double fac_yp=Re_tau*2.0/H;
      double fac_Up=1./utau_;
      
      addPlot
      (
	results, executionPath(), "chartMeanVelocity_"+title,
        "x+", "<U+>",
	list_of<PlotCurve>
	  (PlotCurve( arma::mat(join_rows(fac_yp*data.col(0), fac_Up*data.col(c))), "w l t 'Axial'"))
	  (PlotCurve( arma::mat(join_rows(fac_yp*data.col(0), fac_Up*data.col(c+1))), "w l t 'Wall normal'" ))
	  (PlotCurve( arma::mat(join_rows(fac_yp*data.col(0), fac_Up*data.col(c+2))), "w l t 'Tangential'" )),
	"Longitudinal profiles of averaged velocities at y/H="+lexical_cast<string>(yByH)
      );
    }
    
    // Mean reynolds stress profiles
    {
      double fac_yp=Re_tau*2.0/H;
      double fac_Rp=1./pow(utau_,2);
      int c=cd["UPrime2Mean"].col;
      
      addPlot
      (
	results, executionPath(), "chartMeanRstress_"+title,
        "x+", "<R+>",
	list_of<PlotCurve>
	  (PlotCurve( arma::mat(join_rows(fac_yp*data.col(0), fac_Rp*data.col(c))), "w l t 'Axial'"))
	  (PlotCurve( arma::mat(join_rows(fac_yp*data.col(0), fac_Rp*data.col(c+3))), "w l t 'Wall normal'" ))
	  (PlotCurve( arma::mat(join_rows(fac_yp*data.col(0), fac_Rp*data.col(c+5))), "w l t 'Tangential'" )),
	"Longitudinal profiles of averaged reynolds stresses at y/H="+lexical_cast<string>(yByH)
      );    
    }
  }
    
  for (int i=0; i<ntpc_; i++)
  {
    evaluateAtSection(cm, p, results, ((-0.5+tpc_xlocs_[i])+1e-6)*L, i+1);
    
    const LinearTPCArray* tpcs=cm.get<LinearTPCArray>( string(tpc_names_[i])+"TPCArray");
    if (!tpcs)
      throw insight::Exception("tpc FO array "+string(tpc_names_[i])+" not found in case!");
    tpcs->evaluate(cm, executionPath(), results,
      "two-point correlation of velocity at different radii at x/L="+str(format("%f")%(-0.5+tpc_xlocs_[i]))
    );
  }
  
  return results;
}

void ChannelInflow::applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p)
{
  
  setFields(cm, executionPath(), 
	    list_of<setFieldOps::FieldValueSpec>
	      ("volVectorFieldValue U "+OFDictData::to_OF(vec3(Ubulk_, 0, 0))),
	    ptr_vector<setFieldOps::setFieldOperator>()
  );
  
  cm.get<TurbulentVelocityInletBC>(cycl_in_+"BC")->initInflowBC(executionPath(), p.getSubset("inflow"));
  
  OpenFOAMAnalysis::applyCustomPreprocessing(cm, p);
}

void ChannelInflow::applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts)
{
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  PSDBL(p, "evaluation", mean2time);

  ChannelBase::applyCustomOptions(cm, p, dicts);
  
  OFDictData::dictFile& controlDict=dicts->addDictionaryIfNonexistent("system/controlDict");
  controlDict["endTime"] = (inittime+meantime+mean2time)*T_;
}

addToFactoryTable(Analysis, ChannelInflow, NoParameters);

}
