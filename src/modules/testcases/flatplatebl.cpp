/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "flatplatebl.h"
#include "base/factory.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/openfoamcaseelements.h"
#include "refdata.h"

#include <boost/assign/list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"

using namespace std;
using namespace arma;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight 
{
  
defineType(FlatPlateBL);
addToFactoryTable(Analysis, FlatPlateBL, NoParameters);
  
ParameterSet FlatPlateBL::defaultParameters() const
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
	    ("HBydeltae",		new DoubleParameter(10.0, "Domain height above plate, divided by final BL thickness"))
	    ("WBydeltae",		new DoubleParameter(10.0, "Domain height above plate, divided by final BL thickness"))
	    ("L",		new DoubleParameter(12.0, "[m] Length of the domain"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Geometrical properties of the domain"
	))
      
      ("mesh", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("nh",	new IntParameter(20, "# cells in vertical direction"))
	    ("ypluswall",	new DoubleParameter(0.5, "yPlus of first cell at the wall grid layer at the final station"))
	    ("dxplus",	new DoubleParameter(60, "lateral mesh spacing at the final station"))
	    ("dzplus",	new DoubleParameter(15, "streamwise mesh spacing at the final station"))
	    ("2d",	new BoolParameter(false, "whether to create a two-dimensional grid instead of a three-dimensional one"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Properties of the computational mesh"
	))
      
      ("operation", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("Re_L",		new DoubleParameter(1000, "[-] Reynolds number, formulated with final running length"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Definition of the operation point under consideration"
	))
      
      ("fluid", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("nu",	new DoubleParameter(1.8e-5, "[m^2/s] Viscosity of the fluid"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Parameters of the fluid"
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

FlatPlateBL::FlatPlateBL(const NoParameters&)
: OpenFOAMAnalysis
  (
    "Flat Plate Boundary Layer Test Case",
    "Flat Plate with Evolving Boundary Layer"
  )
{}

FlatPlateBL::~FlatPlateBL()
{

}

void FlatPlateBL::calcDerivedInputData(const ParameterSet& p)
{
  insight::OpenFOAMAnalysis::calcDerivedInputData(p);
  
  PSDBL(p, "geometry", HBydeltae);
  PSDBL(p, "geometry", WBydeltae);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_L);
  PSDBL(p, "fluid", nu);
  PSDBL(p, "mesh", ypluswall);
  PSDBL(p, "mesh", dxplus);
  PSDBL(p, "mesh", dzplus);
  PSINT(p, "mesh", nh);
  
  in_="inlet";
  out_="outlet";
  top_="top";
  cycl_prefix_="cyclic";
  
  Cw_=FlatPlateBL::cw(Re_L);
  cout<<"cw="<<Cw_<<endl;
  delta2e_ = 0.5*Cw_*L;
  cout<<"delta2e="<<delta2e_<<endl;
  H_=HBydeltae*delta2e_;
  cout<<"H="<<H_<<endl;
  W_=WBydeltae*delta2e_;
  cout<<"W="<<W_<<endl;
  Re_theta2e_=Re_L*(delta2e_/L);
  cout<<"Re_theta2e="<<Re_theta2e_<<endl;
  uinf_=Re_L*nu/L;
  cout<<"uinf="<<uinf_<<endl;
  
  cout<<"cf_e="<<cf(Re_L)<<endl;
  ypfac_e_=sqrt(cf(Re_L)/2.)*uinf_/nu;
  cout<<"ypfac="<<ypfac_e_<<endl;
  deltaywall_e_=ypluswall/ypfac_e_;
  cout<<"deltaywall_e="<<deltaywall_e_<<endl;
  gradh_=bmd::GradingAnalyzer(deltaywall_e_, H_, nh).grad();
  cout<<"gradh="<<gradh_<<endl;
  nax_=std::max(1, int(round(L/(dxplus/ypfac_e_))));
  cout<<"nax="<<nax_<<" "<<(dxplus/ypfac_e_)<<endl;
  if (p.getBool("mesh/2d"))
    nlat_=1;
  else
    nlat_=std::max(1, int(round(W_/(dzplus/ypfac_e_))));
  cout<<"nlat="<<nlat_<<" "<<(dzplus/ypfac_e_)<<endl;
  
  T_=L/uinf_;
  cout<<"T="<<T_<<endl;
}

void FlatPlateBL::createMesh(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{
  PSDBL(p, "geometry", HBydeltae);
  PSDBL(p, "geometry", WBydeltae);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_L);
  PSDBL(p, "fluid", nu);
  PSDBL(p, "mesh", ypluswall);
  PSDBL(p, "mesh", dxplus);
  PSDBL(p, "mesh", dzplus);
  PSINT(p, "mesh", nh);
  
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "wall");

  
  std::map<int, Point> pts;
  pts = boost::assign::map_list_of   
      (0, 	vec3(0, 0., 0))
      (1, 	vec3(L, 0., 0))
      (2, 	vec3(L, H_, 0))
      (3, 	vec3(0, H_, 0))
  ;
  
  // create patches
  Patch& in= 	bmd->addPatch(in_, new Patch());
  Patch& out= 	bmd->addPatch(out_, new Patch());
  Patch& top= 	bmd->addPatch(top_, new Patch("symmetryPlane"));
  Patch cycl_side_0=Patch();
  Patch cycl_side_1=Patch();
  
  std::string side_type="cyclic";
  if (p.getBool("mesh/2d")) side_type="empty";
  Patch& cycl_side= 	bmd->addPatch(cycl_prefix_, new Patch(side_type));
  
  arma::mat vH=vec3(0, 0, W_);

#define PTS(a,b,c,d) \
  P_8(pts[a], pts[b], pts[c], pts[d], \
      pts[a]+vH, pts[b]+vH, pts[c]+vH, pts[d]+vH)
      
  {
    Block& bl = bmd->addBlock
    (  
      new Block(PTS(0,1,2,3),
	nax_, nh, nlat_,
	list_of<double>(1.)(gradh_)(1.)
      )
    );
    in.addFace(bl.face("0473"));
    out.addFace(bl.face("1265"));
    top.addFace(bl.face("2376"));
    cycl_side_0.addFace(bl.face("0321"));
    cycl_side_1.addFace(bl.face("4567"));
  }

  cycl_side.appendPatch(cycl_side_0);
  cycl_side.appendPatch(cycl_side_1);
  
  cm.insert(bmd.release());

  cm.createOnDisk(executionPath());
  cm.executeCommand(executionPath(), "blockMesh"); 
}


void FlatPlateBL::createCase(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{
  // create local variables from ParameterSet
  PSDBL(p, "geometry", HBydeltae);
  PSDBL(p, "geometry", WBydeltae);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_L);
  PSDBL(p, "fluid", nu);
  PSDBL(p, "mesh", ypluswall);
  PSDBL(p, "mesh", dxplus);
  PSDBL(p, "mesh", dzplus);
  PSINT(p, "mesh", nh);
  
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
  
//   if (p.getBool("evaluation/eval2"))
//   {
//     cm.insert(new LinearTPCArray(cm, LinearTPCArray::Parameters()
//       .set_name_prefix("tpc_interior")
//       .set_R(0.5*H)
//       .set_x(0.0) // middle x==0!
//       .set_z(-0.49*B)
//       .set_axSpan(0.5*L)
//       .set_tanSpan(0.45*B)
//       .set_timeStart( (inittime+meantime)*T_ )
//     ));
//   }

  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(nu) ));
  
  cm.insert(new VelocityInletBC(cm, in_, boundaryDict, VelocityInletBC::Parameters()
    .set_velocity(vec3(uinf_,0,0))
    .set_turbulenceIntensity(0.005)
    .set_mixingLength(0.1*H_)
  ) );
  cm.insert(new PressureOutletBC(cm, out_, boundaryDict, PressureOutletBC::Parameters()
    .set_pressure(0.0)
  ));
  cm.insert(new SimpleBC(cm, top_, boundaryDict, "symmetryPlane") );
  if (p.getBool("mesh/2d"))
    cm.insert(new SimpleBC(cm, cycl_prefix_, boundaryDict, "empty") );
  else
    cm.insert(new CyclicPairBC(cm, cycl_prefix_, boundaryDict) );
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());
  
  insertTurbulenceModel(cm, p.get<SelectionParameter>("fluid/turbulenceModel").selection());
}

void FlatPlateBL::evaluateAtSection
(
  OpenFOAMCase& cm, const ParameterSet& p, 
  ResultSetPtr results, double x, int i
)
{
  // create local variables from ParameterSet
  PSDBL(p, "geometry", HBydeltae);
  PSDBL(p, "geometry", WBydeltae);
  PSDBL(p, "geometry", L);
  PSDBL(p, "operation", Re_L);
  PSDBL(p, "fluid", nu);
  PSDBL(p, "mesh", ypluswall);
  PSDBL(p, "mesh", dxplus);
  PSDBL(p, "mesh", dzplus);
  PSINT(p, "mesh", nh);
  
  PSDBL(p, "evaluation", inittime);
  PSDBL(p, "evaluation", meantime);
  PSDBL(p, "evaluation", mean2time);

  double xByL= x/L;
  string title="section__xByL_" + str(format("%04.2f") % xByL);
  replace_all(title, ".", "_");
    
  boost::ptr_vector<sampleOps::set> sets;
  
  sets.push_back(new sampleOps::linearAveragedUniformLine(sampleOps::linearAveragedUniformLine::Parameters()
    .set_name("radial")
    .set_start( vec3(x, deltaywall_e_, 0.01*W_))
    .set_end(   vec3(x, H_-deltaywall_e_, 0.01*W_))
    .set_dir1(vec3(1,0,0))
    .set_dir2(vec3(0,0,0.98*W_))
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

  // Mean velocity profiles
  {
    int c=cd["UMean"].col;
    
    arma::mat axial(join_rows(data.col(0)+deltaywall_e_, data.col(c)));
    arma::mat wallnormal(join_rows(data.col(0)+deltaywall_e_, data.col(c+1)));
    arma::mat spanwise(join_rows(data.col(0)+deltaywall_e_, data.col(c+2)));
    
    axial.save( (executionPath()/("umeanaxial_vs_y_"+title+".txt")).c_str(), arma_ascii);
    spanwise.save( (executionPath()/("umeanspanwise_vs_y_"+title+".txt")).c_str(), arma_ascii);
    wallnormal.save( (executionPath()/("umeanwallnormal_vs_y_"+title+".txt")).c_str(), arma_ascii);
    
    addPlot
    (
      results, executionPath(), "chartMeanVelocity_"+title,
      "y", "<U>",
      list_of
      (PlotCurve(axial, "w l lt 1 lc 1 lw 4 t 'Axial'"))
      (PlotCurve(spanwise, "w l lt 1 lc 2 lw 4 t 'Spanwise'"))
      (PlotCurve(wallnormal, "w l lt 1 lc 3 lw 4 t 'Wall normal'"))
      ,
      "Wall normal profiles of averaged velocities at x/L=" + str(format("%g")%xByL),
      "set logscale x"
    );
  }
}

insight::ResultSetPtr FlatPlateBL::evaluateResults(insight::OpenFOAMCase& cm, const insight::ParameterSet& p)
{
  ResultSetPtr results = OpenFOAMAnalysis::evaluateResults(cm, p);
  
  {  
    // Wall friction coefficient
    arma::mat wallforce=viscousForceProfile(cm, executionPath(), vec3(1,0,0), nax_);
      
    arma::mat Cf_vs_x(join_rows(
	wallforce.col(0), 
	wallforce.col(1)/(0.5*pow(uinf_,2))
      ));
    Cf_vs_x.save( (executionPath()/"Cf_vs_x.txt").c_str(), arma_ascii);
    
    arma::mat Cfexp_vs_x=refdatalib.getProfile("Wieghardt1951_FlatPlate", "u17.8/cf_vs_x");

    addPlot
    (
      results, executionPath(), "chartMeanWallFriction",
      "x+", "<Cf>",
      list_of
	(PlotCurve(Cf_vs_x, "w l lt 1 lc 2 lw 2 t 'CFD'"))
	(PlotCurve(Cfexp_vs_x, "w p lt 2 lc 2 t 'Wieghardt/Tillmann 1951 (u=17.8m/s)'"))
	,
      "Axial profile of wall friction coefficient"
    );    
  }
  
  evaluateAtSection(cm, p, results, 0.0,  0);
  evaluateAtSection(cm, p, results, 0.05, 1);
  evaluateAtSection(cm, p, results, 0.1,  2);
  evaluateAtSection(cm, p, results, 0.2,  3);
  evaluateAtSection(cm, p, results, 0.5,  4);
  evaluateAtSection(cm, p, results, 1.0,  5);
  
  return results;
}

insight::Analysis* FlatPlateBL::clone()
{
  return new FlatPlateBL(NoParameters());
}

double FlatPlateBL::G(double Alpha, double D)
{
  struct Obj: public Objective1D
  {
    double Alpha, D;
    virtual double operator()(double G) const 
    { 
//       cout << G << (1./G) + 2.*log(1./G) - D - Alpha <<endl;
      return (Alpha/G) + 2.*log(Alpha/G) - D - Alpha; 
    }
  } obj;
  obj.Alpha=Alpha;
  obj.D=D;
  return nonlinearSolve1D(obj, 1e-6, 1e3*Alpha);
}

double FlatPlateBL::cw(double Re, double Cplus)
{
  return 2.*pow( (0.41/log(Re)) * G( log(Re), 2.*log(0.41)+0.41*(Cplus-3.) ), 2);
}

double FlatPlateBL::cf(double Rex, double Cplus)
{
  struct Obj: public Objective1D
  {
    double Rex, Cplus;
    virtual double operator()(double gamma) const 
    { 
      return (1./gamma) -(1./0.41)*log(gamma*gamma*Rex)
	- Cplus - (1./0.41)*(2.*0.55-log(3.78)); 
    }
  } obj;
  obj.Rex=Rex;
  obj.Cplus=Cplus;
  double gamma=nonlinearSolve1D(obj, 1e-7, 10.);
  return 2.*gamma*gamma;
}

}