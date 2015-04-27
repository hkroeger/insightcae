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
    "Rectangular domain with cyclic BCs on axial ends"
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
	    //("nax",	new IntParameter(100, "# cells in axial direction"))
	    ("nh",	new IntParameter(64, "# cells in vertical direction"))
	    ("fixbuf",	new BoolParameter(false, "fix cell layer size inside buffer layer"))
	    ("dzplus",	new DoubleParameter(15, "Dimensionless grid spacing in spanwise direction"))
	    ("dxplus",	new DoubleParameter(60, "Dimensionless grid spacing in axial direction"))
	    ("ypluswall", new DoubleParameter(2, "yPlus at the wall grid layer"))
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
	    ("eval2", 		new BoolParameter(true, "Whether to evaluate second order statistics"))
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

void ChannelBase::calcDerivedInputData()
{
  const ParameterSet& p=*parameters_;
  
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);

  PSDBL(p, "mesh", ypluswall);
  PSDBL(p, "mesh", dxplus);
  PSDBL(p, "mesh", dzplus);
  PSINT(p, "mesh", nh);
  PSBOOL(p, "mesh", fixbuf);
  
  // Physics
  Re_=Re(Re_tau);
  Ubulk_=Re_/Re_tau;
  T_=L/Ubulk_;
  nu_=1./Re_tau;
  utau_=Re_tau*nu_/(0.5*H);
  ywall_ = ypluswall/Re_tau;
  
  // grid
  //double Delta=L/double(nax);
  nax_=int(L*Re_tau/dxplus);
  
  if (p.getBool("mesh/2d"))
    nb_=1;
  else
    nb_=int(B*Re_tau/dzplus);

  hbuf_=0.0;  
  nh_=nh/2;
  
  nhbuf_=0;
  if (fixbuf>0)
  {
    double ypbuf=30.;
    hbuf_=ypbuf/Re_tau;
    nhbuf_=std::max(1.0, hbuf_/ywall_);
      //ywall_=hbuf_/double(nhbuf);
    
    if (nh_-nhbuf_<=1)
      throw insight::Exception("Cannot fix cell height inside buffer layer: too few cells in vertical direction allowed! (min "+lexical_cast<string>(nhbuf_+1)+")");
  }

  gradh_=bmd::GradingAnalyzer(ywall_, 0.5*H-hbuf_, nh_-nhbuf_).grad();
  //nh_=max(1, bmd::GradingAnalyzer(gradh_).calc_n(ywall_, H/2.));
  

  cout<<"Derived data:"<<endl
      <<"============================================="<<endl;
  cout<<"Reynolds number \tRe="<<Re_<<endl;
  cout<<"Bulk velocity \tUbulk="<<Ubulk_<<endl;
  cout<<"Flow-through time \tT="<<T_<<endl;
  cout<<"Viscosity \tnu="<<nu_<<endl;
  cout<<"Height of buffer layer\thbuf="<<hbuf_<<endl;
  cout<<"No cells in buffer layer\tnhbuf="<<nhbuf_<<endl;
  cout<<"Friction velocity \tutau="<<utau_<<endl;
  cout<<"Wall distance of first grid point \tywall="<<ywall_<<endl;
  cout<<"# cells axial \tnax="<<nax_<<endl;
  cout<<"# cells spanwise \tnb="<<nb_<<endl;
  cout<<"# grading vertical \tgradh="<<gradh_<<endl;
  cout<<"============================================="<<endl;
}

void ChannelBase::createMesh
(
  OpenFOAMCase& cm
)
{  
  // create local variables from ParameterSet
  path dir = executionPath();
  const ParameterSet& p=*parameters_;
  
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  
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
      .convert_to_container<std::map<int, Point> >()
  ;
  
  // create patches
  Patch& cycl_in= 	bmd->addPatch(cycl_in_, new Patch());
  Patch& cycl_out= 	bmd->addPatch(cycl_out_, new Patch());
  Patch cycl_side_0=Patch();
  Patch cycl_side_1=Patch();
  
  string side_type="cyclic";
  if (p.getBool("mesh/2d")) side_type="empty";
  Patch& cycl_side= 	bmd->addPatch("cycl_side", new Patch(side_type));
  
  arma::mat vHbuf=vec3(0, 0, 0);
  arma::mat vH=vec3(0, H, 0);
  int nh=nh_;
  
  if (nhbuf_>0)
  {
    vHbuf=vec3(0, hbuf_, 0);
    vH=vec3(0, H-2.*hbuf_, 0);
    nh=nh_-nhbuf_;
    
    {
      Block& bl = bmd->addBlock
      (  
	new Block(P_8(
	  pts[0], pts[1], pts[2], pts[3],
	  (pts[0])+vHbuf, (pts[1])+vHbuf, (pts[2])+vHbuf, (pts[3])+vHbuf
	  ),
	  nax_, nb_, nhbuf_,
	  list_of<double>(1.)(1.)(1.)
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
	    (pts[0])+vH+vHbuf, (pts[1])+vH+vHbuf, (pts[2])+vH+vHbuf, (pts[3])+vH+vHbuf,
	    (pts[0])+vH+2.*vHbuf, (pts[1])+vH+2.*vHbuf, (pts[2])+vH+2.*vHbuf, (pts[3])+vH+2.*vHbuf
	  ),
	  nax_, nb_, nhbuf_,
	  list_of<double>(1.)(1.)(1.)
	)
      );
      cycl_out.addFace(bl.face("0473"));
      cycl_in.addFace(bl.face("1265"));
      cycl_side_0.addFace(bl.face("0154"));
      cycl_side_1.addFace(bl.face("2376"));
    }
    
  }

  {
    Block& bl = bmd->addBlock
    (  
      new Block(P_8(
	  pts[0]+vHbuf, pts[1]+vHbuf, pts[2]+vHbuf, pts[3]+vHbuf,
	  (pts[0])+0.5*vH+vHbuf, (pts[1])+0.5*vH+vHbuf, (pts[2])+0.5*vH+vHbuf, (pts[3])+0.5*vH+vHbuf
	),
	nax_, nb_, nh,
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
	  (pts[0])+0.5*vH+vHbuf, (pts[1])+0.5*vH+vHbuf, (pts[2])+0.5*vH+vHbuf, (pts[3])+0.5*vH+vHbuf,
	  (pts[0])+vH+vHbuf, (pts[1])+vH+vHbuf, (pts[2])+vH+vHbuf, (pts[3])+vH+vHbuf
	),
	nax_, nb_, nh,
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
  OpenFOAMCase& cm
)
{
  const ParameterSet& p=*parameters_;

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
    .set_hasCyclics(true)
  ) );
  cm.insert(new extendedForces(cm, extendedForces::Parameters()
    .set_patches( list_of<string>("walls") )
  ));
  cm.insert(new fieldAveraging(cm, fieldAveraging::Parameters()
    .set_name("zzzaveraging") // shall be last FO in list
    .set_fields(list_of<std::string>("p")("U")("pressureForce")("viscousForce"))
    .set_timeStart(inittime*T_)
  ));
  
  if (p.getBool("evaluation/eval2"))
  {
    cm.insert(new LinearTPCArray(cm, LinearTPCArray::Parameters()
      .set_name_prefix("tpc_interior")
      .set_R(0.5*H)
      .set_x(0.0) // middle x==0!
      .set_z(-0.49*B)
      .set_axSpan(0.5*L)
      .set_tanSpan(0.45*B)
      .set_timeStart( (inittime+meantime)*T_ )
    ));
  }

  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(nu_) ));
  if (p.getBool("mesh/2d"))
    cm.insert(new SimpleBC(cm, "cycl_side", boundaryDict, "empty") );
  else
    cm.insert(new CyclicPairBC(cm, "cycl_side", boundaryDict) );
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());
  
  insertTurbulenceModel(cm, p.get<SelectionParameter>("fluid/turbulenceModel").selection());

}

void ChannelBase::applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts)
{
  OpenFOAMAnalysis::applyCustomOptions(cm, dicts);
  
//   OFDictData::dictFile& controlDict=dicts->addDictionaryIfNonexistent("system/controlDict");
//   controlDict["maxDeltaT"]=0.5*T_;
}

void ChannelBase::evaluateAtSection(
  OpenFOAMCase& cm, 
  ResultSetPtr results, double x, int i
)
{
  const ParameterSet& p=*parameters_;

  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  
  double xByH= (x/L + 0.5)*L/H;
  string title="section__xByH_" + str(format("%04.2f") % xByH);
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
  arma::mat data = dynamic_cast<sampleOps::linearAveragedUniformLine*>(&sets[0])
    ->readSamples(cm, executionPath(), &cd);
    
    cout<<data<<endl;
    BOOST_FOREACH(const sampleOps::ColumnDescription::value_type& c, cd)
     cout<<c.first<<" "<<c.second.col<<endl;
      
  arma::mat refdata_umean180=refdatalib.getProfile("MKM_Channel", "180/umean_vs_yp");
  arma::mat refdata_wmean180=refdatalib.getProfile("MKM_Channel", "180/wmean_vs_yp");
  arma::mat refdata_umean395=refdatalib.getProfile("MKM_Channel", "395/umean_vs_yp");
  arma::mat refdata_wmean395=refdatalib.getProfile("MKM_Channel", "395/wmean_vs_yp");
  arma::mat refdata_umean590=refdatalib.getProfile("MKM_Channel", "590/umean_vs_yp");
  arma::mat refdata_wmean590=refdatalib.getProfile("MKM_Channel", "590/wmean_vs_yp");

  // Mean velocity profiles
  {
    int c=cd["UMean"].col;
    
    arma::mat axial(join_rows(Re_tau-Re_tau*data.col(0), data.col(c)));
    arma::mat wallnormal(join_rows(Re_tau-Re_tau*data.col(0), data.col(c+1)));
    arma::mat spanwise(join_rows(Re_tau-Re_tau*data.col(0), data.col(c+2)));
    
    axial.save( (executionPath()/("umeanaxial_vs_yp_"+title+".txt")).c_str(), arma::raw_ascii);
    spanwise.save( (executionPath()/("umeanspanwise_vs_yp_"+title+".txt")).c_str(), arma::raw_ascii);
    wallnormal.save( (executionPath()/("umeanwallnormal_vs_yp_"+title+".txt")).c_str(), arma::raw_ascii);
    
    addPlot
    (
      results, executionPath(), "chartMeanVelocity_"+title,
      "y+", "<U+>",
      list_of
      (PlotCurve(axial, "w l lt 1 lc 1 lw 4 t 'Axial'"))
      (PlotCurve(spanwise, "w l lt 1 lc 2 lw 4 t 'Spanwise'"))
      (PlotCurve(wallnormal, "w l lt 1 lc 3 lw 4 t 'Wall normal'"))
      (PlotCurve(refdata_umean180, "w l lt 2 lc 1 t 'Axial (MKM Re_tau=180)'"))
      (PlotCurve(refdata_wmean180, "w l lt 2 lc 2 t 'Spanwise (MKM Re_tau=180)'"))
      (PlotCurve(refdata_umean395, "w l lt 4 lc 1 t 'Axial (MKM Re_tau=395)'"))
      (PlotCurve(refdata_wmean395, "w l lt 4 lc 2 t 'Spanwise (MKM Re_tau=395)'"))
      (PlotCurve(refdata_umean590, "w l lt 3 lc 1 t 'Axial (MKM Re_tau=590)'"))
      (PlotCurve(refdata_wmean590, "w l lt 3 lc 2 t 'Spanwise (MKM Re_tau=590)'"))
      ,
      "Wall normal profiles of averaged velocities at x/H=" + str(format("%g")%xByH),
      "set logscale x"
    );
    
  }
  
  // L profiles from k/omega
  if ((cd.find("k")!=cd.end()) && (cd.find("omega")!=cd.end()))
  {    
    arma::mat k=data.col(cd["k"].col);
    arma::mat omega=data.col(cd["omega"].col);
    
    arma::mat ydelta=1.0-(data.col(0)+delta_yp1)/(0.5*H); //Re_tau-Re_tau*data.col(0);
    arma::mat Lt1=(2./H)*sqrt(k)/(0.09*omega);
    arma::mat Lt2=ydelta*0.5*H*0.41;
    //arma::mat Lt=arma::min(Lt1, Lt2);
    arma::mat Lt=Lt1;
    for (int i=0; i<Lt2.n_rows; i++) Lt(i)=min(Lt(i), Lt2(i));
    arma::mat Ltp(join_rows(ydelta, Lt));
    Ltp.save( (executionPath()/("LdeltaRANS_vs_yp_"+title+".txt")).c_str(), arma::raw_ascii);
    
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
    
    addPlot
    (
      results, executionPath(), "chartTurbulentLengthScale_"+title,
      "y_delta", "<L_delta_RANS>",
      list_of
       (PlotCurve(arma::mat(join_rows(ydelta, Lt1)), "w l lt 2 lc 1 lw 1 t 'CFD (from k and omega)'"))
       (PlotCurve(arma::mat(join_rows(ydelta, Lt2)), "w l lt 3 lc 1 lw 1 t 'Mixing length limit'"))
       (PlotCurve(Ltp, "w l lt 1 lc 1 lw 2 t 'CFD'"))
       (PlotCurve(arma::mat(join_rows(ydelta, yfit)), 
		    "w l lt 2 lc 2 lw 2 t 'Fit "
		    + 	    str(format("%.1g") % m.c0)+"*ydelta^"+str(format("%.1g") % m.c2)
		    +" + ("+str(format("%.1g") % m.c1)+"*ydelta^"+str(format("%.1g") % m.c3)+")'"))
       ,
      "Wall normal profile of turbulent length scale at x/H=" + str(format("%g")%xByH)
    );

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
       
  }

  arma::mat refdata_Ruu=refdatalib.getProfile("MKM_Channel", "180/Ruu_vs_yp");
  arma::mat refdata_Rvv=refdatalib.getProfile("MKM_Channel", "180/Rvv_vs_yp");
  arma::mat refdata_Rww=refdatalib.getProfile("MKM_Channel", "180/Rww_vs_yp");
  arma::mat refdata_Ruu395=refdatalib.getProfile("MKM_Channel", "395/Ruu_vs_yp");
  arma::mat refdata_Rvv395=refdatalib.getProfile("MKM_Channel", "395/Rvv_vs_yp");
  arma::mat refdata_Rww395=refdatalib.getProfile("MKM_Channel", "395/Rww_vs_yp");
  arma::mat refdata_Ruu590=refdatalib.getProfile("MKM_Channel", "590/Ruu_vs_yp");
  arma::mat refdata_Rvv590=refdatalib.getProfile("MKM_Channel", "590/Rvv_vs_yp");
  arma::mat refdata_Rww590=refdatalib.getProfile("MKM_Channel", "590/Rww_vs_yp");
  
  arma::mat refdata_K=refdata_Ruu;
  refdata_K.col(1)+=Interpolator(refdata_Rvv)(refdata_Ruu.col(0));
  refdata_K.col(1)+=Interpolator(refdata_Rww)(refdata_Ruu.col(0));
  refdata_K.col(1)*=0.5;
  refdata_K.col(0)/=180.0;
  
  arma::mat refdata_K395=refdata_Ruu395;
  refdata_K395.col(1)+=Interpolator(refdata_Rvv395)(refdata_Ruu395.col(0));
  refdata_K395.col(1)+=Interpolator(refdata_Rww395)(refdata_Ruu395.col(0));
  refdata_K395.col(1)*=0.5;
  refdata_K395.col(0)/=395.0;

  arma::mat refdata_K590=refdata_Ruu590;
  refdata_K590.col(1)+=Interpolator(refdata_Rvv590)(refdata_Ruu590.col(0));
  refdata_K590.col(1)+=Interpolator(refdata_Rww590)(refdata_Ruu590.col(0));
  refdata_K590.col(1)*=0.5;
  refdata_K590.col(0)/=590.0;
  

  // Mean reynolds stress profiles
  {
    string chart_name="chartMeanReyStress_"+title;
    
    int c=cd["UPrime2Mean"].col;
    arma::mat axial(join_rows(Re_tau-Re_tau*data.col(0), data.col(c)));
    arma::mat spanwise(join_rows(Re_tau-Re_tau*data.col(0), data.col(c+3)));
    arma::mat wallnormal(join_rows(Re_tau-Re_tau*data.col(0), data.col(c+5)));
    
    addPlot
    (
      results, executionPath(), chart_name,
      "y+", "<R+>",
      list_of
       (PlotCurve(axial, "w l lt 1 lc 1 lw 4 t 'Rxx (Axial)'"))
       (PlotCurve(spanwise, "w l lt 1 lc 2 lw 4 t 'Ryy (Spanwise)'"))
       (PlotCurve(wallnormal, "w l lt 1 lc 3 lw 4 t 'Rzz (Wall normal)'"))
       (PlotCurve(refdata_Ruu, "w l lt 2 lc 1 t 'Rxx (MKM Re_tau=180)'"))
       (PlotCurve(refdata_Rvv, "w l lt 2 lc 2 t 'Ryy (MKM Re_tau=180)'"))
       (PlotCurve(refdata_Rww, "w l lt 2 lc 3 t 'Rzz (MKM Re_tau=180)'"))
       (PlotCurve(refdata_Ruu395, "w l lt 4 lc 1 t 'Rxx (MKM Re_tau=395)'"))
       (PlotCurve(refdata_Rvv395, "w l lt 4 lc 2 t 'Ryy (MKM Re_tau=395)'"))
       (PlotCurve(refdata_Rww395, "w l lt 4 lc 3 t 'Rzz (MKM Re_tau=395)'"))
       (PlotCurve(refdata_Ruu590, "w l lt 3 lc 1 t 'Rxx (MKM Re_tau=590)'"))
       (PlotCurve(refdata_Rvv590, "w l lt 3 lc 2 t 'Ryy (MKM Re_tau=590)'"))
       (PlotCurve(refdata_Rww590, "w l lt 3 lc 3 t 'Rzz (MKM Re_tau=590)'"))
       ,
     "Wall normal profiles of averaged reynolds stresses at x/H=" + str(format("%g")%xByH),
     "set yrange [:"+lexical_cast<string>(max(axial.col(1)))+"]"
    );

    chart_name="chartMeanTKE_"+title;
    
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
    Kp.save( (executionPath()/("Kp_vs_ydelta_"+title+".txt")).c_str(), arma::raw_ascii);
    
    addPlot
    (
      results, executionPath(), chart_name,
      "y_delta", "<K+>",
      list_of
       (PlotCurve( Kp, "w l t 'TKE'" ))
       (PlotCurve( refdata_K, "u 1:2 w l lt 1 lc 1 t 'DNS (Re_tau=180, MKM)'" ))
       (PlotCurve( refdata_K395, "u 1:2 w l lt 2 lc 1 t 'DNS (Re_tau=395, MKM)'" ))
       (PlotCurve( refdata_K590, "u 1:2 w l lt 3 lc 1 t 'DNS (Re_tau=590, MKM)'" ))
       ,
     "Wall normal profiles of averaged turbulent kinetic energy (1/2 R_ii + k_model) at x/H=" + str(format("%g")%xByH)
    );
  }

  std::string init=
      "cbi=loadOFCase('.')\n"
      "prepareSnapshots()\n";
      
  if (!p.getBool("mesh/2d"))
  {
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
      executionPath(), pressure_contour_filename, 
      "Contour of pressure (axial section at x/H=" + str(format("%g")%xByH)+")", ""
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
	executionPath(), velocity_contour_filename, 
	"Contour of "+c+"-Velocity (axial section at x/H=" + str(format("%g")%xByH)+")", ""
      )));
    }
  }
}

ResultSetPtr ChannelBase::evaluateResults(OpenFOAMCase& cm)
{
  const ParameterSet& p=*parameters_;
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  
  ResultSetPtr results = OpenFOAMAnalysis::evaluateResults(cm);
  
  evaluateAtSection(cm, results, 1e-3, 0);

  const LinearTPCArray* tpcs=cm.get<LinearTPCArray>("tpc_interiorTPCArray");
  if (!tpcs)
  {
    //throw insight::Exception("tpc FO array not found in case!");
  }
  else
  {
    tpcs->evaluate
    (
      cm, executionPath(), results, 
      "two-point correlation of velocity at different radii at x/H="+str(format("%g")%(0.5*L/H))
    );
  }
 
  try
  {
    
   // Wall friction coefficient
   arma::mat wallforce=viscousForceProfile(cm, executionPath(), vec3(1,0,0), nax_);
    
   arma::mat Cf_vs_xp(join_rows(
      wallforce.col(0)*Re_tau, 
      wallforce.col(1)/(0.5*pow(Ubulk_,2))
    ));
    Cf_vs_xp.save( (executionPath()/"Cf_vs_xp.txt").c_str(), arma::raw_ascii);
    
    arma::mat Cftheo_vs_xp=zeros(2,2);
    Cftheo_vs_xp(0,0)=Cf_vs_xp(0,0);
    Cftheo_vs_xp(1,0)=Cf_vs_xp(Cf_vs_xp.n_rows-1,0);
    double Cftheo=pow(utau_,2)/(0.5*pow(Ubulk_,2));
    Cftheo_vs_xp(0,1)=Cftheo;
    Cftheo_vs_xp(1,1)=Cftheo;

    addPlot
    (
      results, executionPath(), "chartMeanWallFriction",
      "x+", "<Cf>",
      list_of
	(PlotCurve(Cf_vs_xp, "w l lt 1 lc 2 lw 2 t 'CFD'"))
	(PlotCurve(Cftheo_vs_xp, "w l lt 2 lc 2 lw 1 t 'Analytical'"))
	,
      "Axial profile of wall friction coefficient"
    );    
  }
  catch (...)
  {
    insight::Warning("Could not include viscous resistance coefficient plot into result report.\nCheck console output for reason.");
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
    executionPath(), "pressure_longi.jpg", 
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
      executionPath(), "U"+c+"_longi.jpg", 
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
  OpenFOAMCase& cm
)
{  
  ChannelBase::createMesh(cm);
  convertPatchPairToCyclic(cm, executionPath(), cyclPrefix());
}

void ChannelCyclic::createCase
(
  OpenFOAMCase& cm
)
{  
  const ParameterSet& p=*parameters_;
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
  
  ChannelBase::createCase(cm);
  
  {
    std::vector<arma::mat> pl;
    double l=0.25*H;
    int np=25;
    for (int j=0; j<np; j++)
    {
      pl.push_back(vec3(l*double(j)/double(np-1), 0, 0));
    }
    cm.insert(new probes(cm, probes::Parameters()
    .set_name("center_probes")
    .set_fields( list_of<std::string>("p")("U") )
    .set_probeLocations(pl)
    ));
  }
  
  cm.insert(new PressureGradientSource(cm, PressureGradientSource::Parameters()
					    .set_Ubar(vec3(Ubulk_, 0, 0))
		));

}

void ChannelCyclic::applyCustomPreprocessing(OpenFOAMCase& cm)
{
  const ParameterSet& p=*parameters_;

  if (p.getBool("run/perturbU"))
  {
    PSDBL(p, "operation", Re_tau);

    cm.executeCommand(executionPath(), "perturbU", 
		      list_of<string>
		      (lexical_cast<string>(Re_tau))
		      ("("+lexical_cast<string>(Ubulk_)+" 0 0)") 
		    );
  }
  OpenFOAMAnalysis::applyCustomPreprocessing(cm);
}

void ChannelCyclic::applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts)
{
  const ParameterSet& p=*parameters_;
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  PSDBL(p, "evaluation", mean2time);

  ChannelBase::applyCustomOptions(cm, dicts);
  
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
  
  std::auto_ptr<SubsetParameter> inflowparams(new SubsetParameter(TurbulentVelocityInletBC::Parameters::makeDefault(), "Inflow BC"));
  
//   (*inflowparams)().extend
//   (
//       boost::assign::list_of<ParameterSet::SingleEntry>
//       ("umean", FieldData::defaultParameter(vec3(1,0,0)))
//       .convert_to_container<ParameterSet::EntryList>()
//   );
  
  p.extend
  (
    boost::assign::list_of<ParameterSet::SingleEntry>
    ("inflow", inflowparams.release())
    .convert_to_container<ParameterSet::EntryList>()
  );

  return p;
}

void ChannelInflow::createMesh
(
  OpenFOAMCase& cm
)
{  
  ChannelBase::createMesh(cm);
  //convertPatchPairToCyclic(cm, executionPath(), cyclPrefix());
}

void ChannelInflow::createCase
(
  OpenFOAMCase& cm
)
{  
  const ParameterSet& p=*parameters_;
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
      
  cm.insert(new TurbulentVelocityInletBC( cm, cycl_in_, boundaryDict, p.getSubset("inflow") ));
  
  cm.insert(new PressureOutletBC(cm, cycl_out_, boundaryDict, PressureOutletBC::Parameters()
    .set_pressure(0.0)
    .set_fixMeanValue(true)
  ));
  
  ChannelBase::createCase(cm);
  
  if (p.getBool("evaluation/eval2"))
  {
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
  
}

ResultSetPtr ChannelInflow::evaluateResults(OpenFOAMCase& cm)
{
  const ParameterSet& p=*parameters_;
  PSDBL(p, "geometry", H);
  PSDBL(p, "geometry", B);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_tau);
  
  ResultSetPtr results = ChannelBase::evaluateResults(cm);
  
  {
    
   // Pressure fluctuations
   arma::mat pPrime=interiorPressureFluctuationProfile(cm, executionPath(), vec3(1,0,0), nax_);
    
   arma::mat pPrime2_vs_xp(join_rows(
      pPrime.col(0)*Re_tau, 
      pPrime.col(1)
    ));
    pPrime2_vs_xp.save( (executionPath()/"pPrime2_vs_xp.txt").c_str(), raw_ascii);
    
    addPlot
    (
      results, executionPath(), "chartMeanpPrime2",
      "x+", "<pPrime^2>",
      list_of
	(PlotCurve(pPrime2_vs_xp, "w l lt 1 lc 2 lw 2 not"))
	,
      "Axial profile of pressure fluctuation",
      "set logscale y;"
    );    
  }
  
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
	"Longitudinal profiles of averaged velocities at y/H="+str(format("%g")%yByH)
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
	"Longitudinal profiles of averaged reynolds stresses at y/H="+str(format("%g")%yByH)
      );    
    }
  }
    
  for (int i=0; i<ntpc_; i++)
  {
    evaluateAtSection(cm, results, ((-0.5+tpc_xlocs_[i])+1e-6)*L, i+1);
    
    const LinearTPCArray* tpcs=cm.get<LinearTPCArray>( string(tpc_names_[i])+"TPCArray");
    
    if (!tpcs)
    {
      //throw insight::Exception("tpc FO array not found in case!");
    }
    else
    {
      tpcs->evaluate(cm, executionPath(), results,
	"two-point correlation of velocity at different radii at x/L="+str(format("%f")%(-0.5+tpc_xlocs_[i]))
	    );
    }
    
  }
  
  return results;
}

void ChannelInflow::applyCustomPreprocessing(OpenFOAMCase& cm)
{
  
  setFields(cm, executionPath(), 
	    list_of<setFieldOps::FieldValueSpec>
	      ("volVectorFieldValue U "+OFDictData::to_OF(vec3(Ubulk_, 0, 0))),
	    ptr_vector<setFieldOps::setFieldOperator>()
  );
  
//   cm.get<TurbulentVelocityInletBC>(cycl_in_+"BC")->initInflowBC(executionPath(), p.getSubset("inflow"));
  
  OpenFOAMAnalysis::applyCustomPreprocessing(cm);
}

void ChannelInflow::applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts)
{
  PSDBL(p(), "evaluation", inittime);
  PSDBL(p(), "evaluation", meantime);
  PSDBL(p(), "evaluation", mean2time);

  ChannelBase::applyCustomOptions(cm, dicts);
  
  OFDictData::dictFile& controlDict=dicts->addDictionaryIfNonexistent("system/controlDict");
  controlDict["endTime"] = (inittime+meantime+mean2time)*T_;
}

addToFactoryTable(Analysis, ChannelInflow, NoParameters);

}
