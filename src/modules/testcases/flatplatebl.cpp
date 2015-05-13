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

const std::vector<double> FlatPlateBL::sec_locs_ 
 = list_of (0.01)(0.05)(0.1)(0.2)(0.5)(0.7)(0.9);
  
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
	    ("HBydeltae",		new DoubleParameter(60.0, "Domain height above plate, divided by final BL thickness"))
	    ("WBydeltae",		new DoubleParameter(20.0, "Domain height above plate, divided by final BL thickness"))
	    ("L",		new DoubleParameter(5.0, "[m] Length of the domain"))
	    ("LapByL",		new DoubleParameter(0.1, "Length of the approach zone, divided by length of plate"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Geometrical properties of the domain"
	))
      
      ("mesh", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("nh",	new IntParameter(50, "# cells in vertical direction"))
	    ("ypluswall",	new DoubleParameter(1, "yPlus of first cell at the wall grid layer at the final station"))
	    ("dxplus",	new DoubleParameter(1000, "lateral mesh spacing at the final station"))
	    ("dzplus",	new DoubleParameter(1000, "streamwise mesh spacing at the final station"))
	    ("2d",	new BoolParameter(true, "select method of transition enforcement"))
	    
	    ("tripping", 	new SelectableSubsetParameter("none", 
		  list_of<SelectableSubsetParameter::SingleSubset>
		  (
		    "none", new ParameterSet(ParameterSet::EntryList())
		  )
		  (
		    "blocks", new ParameterSet
		    (
		      list_of<ParameterSet::SingleEntry>
// 		      ("n", 	new IntParameter(4, "number evenly distributed tripping block across plate width"))
		      ("wbyh",  new DoubleParameter(6, "width of the blocks"))
		      ("lbyh", 	new DoubleParameter(3, "length of the blocks"))
		      ("Reh", 	new DoubleParameter(1000, "Reynolds number (formulated with height) of the tripping box"))
		      .convert_to_container<ParameterSet::EntryList>()
		    )
		  )
		  (
		    "drag", new ParameterSet
		    (
		      list_of<ParameterSet::SingleEntry>
		      ("CD",  new DoubleParameter(1, "Drag coefficient"))
		      ("lbyh", 	new DoubleParameter(3, "length of the blocks"))
		      ("Reh", 	new DoubleParameter(1000, "Reynolds number (formulated with height) of the tripping zone"))
		      .convert_to_container<ParameterSet::EntryList>()
		    )
		  )
		  ,
		  "Tripping from laminar to turbulent flow")
	    )

	    ("gradaxi",	new DoubleParameter(50, "grading from plate beginning towards inlet boundary"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Properties of the computational mesh"
	))
      
//       ("operation", new SubsetParameter
// 	(
// 	  ParameterSet
// 	  (
// 	    boost::assign::list_of<ParameterSet::SingleEntry>
// 	    ("Re_L",		new DoubleParameter(1000, "[-] Reynolds number, formulated with final running length"))
// 	    ("prof",		FieldData::defaultParameter(vec3(1,0,0)))
// 	    .convert_to_container<ParameterSet::EntryList>()
// 	  ), 
// 	  "Definition of the operation point under consideration"
// 	))
      
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

      ("run", new SubsetParameter	
	    (
		  ParameterSet
		  (
		    boost::assign::list_of<ParameterSet::SingleEntry>
// 		    ("iter", 	new IntParameter(30000, "number of outer iterations after which the solver should stop"))
		    ("regime", 	new SelectableSubsetParameter("steady", 
			  list_of<SelectableSubsetParameter::SingleSubset>
			  (
			    "steady", new ParameterSet
			    (
			      list_of<ParameterSet::SingleEntry>
			      ("iter", 	new IntParameter(1000, "number of outer iterations after which the solver should stop"))
			      .convert_to_container<ParameterSet::EntryList>()
			    )
			  )
			  (
			    "unsteady", new ParameterSet
			    (
			      list_of<ParameterSet::SingleEntry>
			      ("inittime",	new DoubleParameter(10, "[T] length of grace period before averaging starts (as multiple of flow-through time)"))
			      ("meantime",	new DoubleParameter(10, "[T] length of time period for averaging of velocity and RMS (as multiple of flow-through time)"))
			      ("mean2time",	new DoubleParameter(10, "[T] length of time period for averaging of second order statistics (as multiple of flow-through time)"))
			      .convert_to_container<ParameterSet::EntryList>()
			    )
			  ),
			  "The simulation regime"))
		    .convert_to_container<ParameterSet::EntryList>()
		  ), 
		  "Solver parameters"
      ))
      
      .convert_to_container<ParameterSet::EntryList>()
  );

  std::auto_ptr<SubsetParameter> inflowparams
  (
    new SubsetParameter
    (
      TurbulentVelocityInletBC::Parameters::makeDefault(), 
      "Inflow BC"
    )
  );
  p.extend
  (
    boost::assign::list_of<ParameterSet::SingleEntry>
    ("inflow", inflowparams.release())
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


void FlatPlateBL::calcDerivedInputData()
{
  insight::OpenFOAMAnalysis::calcDerivedInputData();
  const ParameterSet& p=*parameters_;
  
  PSDBL(p, "geometry", HBydeltae);
  PSDBL(p, "geometry", WBydeltae);
  PSDBL(p, "geometry", L);
  PSDBL(p, "geometry", LapByL);
  PSDBL(p, "fluid", nu);
  PSDBL(p, "mesh", ypluswall);
  PSDBL(p, "mesh", dxplus);
  PSDBL(p, "mesh", dzplus);
  PSINT(p, "mesh", nh);
  
  in_="inlet";
  out_="outlet";
  top_="top";
  cycl_prefix_="cyclic";
  approach_="approach";
  trip_="trip";
  
  uinf_=FieldData(p.getSubset("inflow").getSubset("umean")).maxValueMag();
  Re_L_=uinf_*L/nu;
  
  dtrip_=1000.*nu/uinf_; // Re(d_tripwire)=1000
  std::cout<<"dtrip="<<dtrip_<<std::endl;
  
  Cw_=FlatPlateBL::cw(Re_L_);
  cout<<"cw="<<Cw_<<endl;
  delta2e_ = 0.5*Cw_*L;
  cout<<"delta2e="<<delta2e_<<endl;
  H_=HBydeltae*delta2e_;
  cout<<"H="<<H_<<endl;
  W_=WBydeltae*delta2e_;
  cout<<"W="<<W_<<endl;
  Re_theta2e_=Re_L_*(delta2e_/L);
  cout<<"Re_theta2e="<<Re_theta2e_<<endl;
//   uinf_=Re_L*nu/L;
//   cout<<"uinf="<<uinf_<<endl;
  
  cout<<"cf_e="<<cf(Re_L_)<<endl;
  ypfac_e_=sqrt(cf(Re_L_)/2.)*uinf_/nu;
  cout<<"ypfac="<<ypfac_e_<<endl;
  deltaywall_e_=ypluswall/ypfac_e_;
  cout<<"deltaywall_e="<<deltaywall_e_<<endl;
  gradh_=bmd::GradingAnalyzer(deltaywall_e_, H_, nh).grad();
  cout<<"gradh="<<gradh_<<endl;
  double deltax=(dxplus/ypfac_e_);
  gradax_=deltax/dtrip_;
//   nax_=std::max(1, int(round(L/deltax)));
  nax_=bmd::GradingAnalyzer(gradax_).calc_n(dtrip_, L);
  cout<<"nax="<<nax_<<" "<<(dxplus/ypfac_e_)<<endl;

  gradaxi_=p.getDouble("mesh/gradaxi");
//   naxi_=std::max(1, int(round(0.1*L/deltax)));
  naxi_=bmd::GradingAnalyzer(gradaxi_).calc_n(dtrip_, LapByL*L);
  cout<<"naxi="<<naxi_<<endl;
  if (p.getBool("mesh/2d"))
    nlat_=1;
  else
    nlat_=std::max(1, int(round(W_/(dzplus/ypfac_e_))));
  cout<<"nlat="<<nlat_<<" "<<(dzplus/ypfac_e_)<<endl;
  
  T_=L/uinf_;
  cout<<"T="<<T_<<endl;
  
  std::string regime = p.get<SelectableSubsetParameter>("run/regime").selection();
  if (regime=="steady")
  {
    end_=p.getInt("run/regime/iter");
    avgStart_=0.98*end_;
    avg2Start_=end_;
  } 
  else if (regime=="unsteady")
  {
    avgStart_=p.getDouble("run/regime/inittime")*T_;
    avg2Start_=avgStart_+p.getDouble("run/regime/meantime")*T_;
    end_=avg2Start_+p.getDouble("run/regime/mean2time")*T_;
  }

}

void FlatPlateBL::createMesh(insight::OpenFOAMCase& cm)
{
  const ParameterSet& p=*parameters_;
  
  PSDBL(p, "geometry", HBydeltae);
  PSDBL(p, "geometry", WBydeltae);
  PSDBL(p, "geometry", L);
  PSDBL(p, "geometry", LapByL);
//   PSDBL(p, "operation", Re_L);
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
      
      (4, 	vec3(-LapByL*L, 0., 0))
      (5, 	vec3(-LapByL*L, H_, 0))
      .convert_to_container<std::map<int, Point> >()
  ;
  
  // create patches
  Patch& approach= bmd->addPatch(approach_, new Patch());
  Patch& in= 	bmd->addPatch(in_, new Patch());
  Patch& out= 	bmd->addPatch(out_, new Patch());
  Patch& top= 	bmd->addPatch(top_, new Patch(/*"symmetryPlane"*/));
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
      new Block(PTS(4,0,3,5),
	naxi_, nh, nlat_,
	list_of<double>(1./gradaxi_)(gradh_)(1.),
	approach_
      )
    );
    in.addFace(bl.face("0473"));
    approach.addFace(bl.face("0154"));
//     out.addFace(bl.face("1265"));
    top.addFace(bl.face("2376"));
    cycl_side_0.addFace(bl.face("0321"));
    cycl_side_1.addFace(bl.face("4567"));
  }

  {
    Block& bl = bmd->addBlock
    (  
      new Block(PTS(0,1,2,3),
	nax_, nh, nlat_,
	list_of<double>(gradax_)(gradh_)(1.)
      )
    );
//     in.addFace(bl.face("0473"));
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
  
  const SelectableSubsetParameter& tp = p.get<SelectableSubsetParameter>("mesh/tripping");
  if (tp.selection()=="blocks")
  {
//     int n=tp().getInt("n");
    double Reh=tp().getDouble("Reh");
    double wbyh=tp().getDouble("wbyh");
    double lbyh=tp().getDouble("lbyh");
    
    double dtrip=Reh*nu/uinf_;
    double w=wbyh*dtrip; //W_/double(2*n);
    int n=floor(W_/w/2.);
    double Ltrip=lbyh*dtrip; //3.*dtrip;
    
    std::vector<std::string> cmds;
    cmds.push_back( str( format("cellSet %s new boxToCell (-1e-10 0 -1e10) (%g %g 1e10)") 
	% trip_ 
	% Ltrip % /*dtrip */0.0
    ) );
    for (int i=0; i<n; i++)
    {
      double w0=(double(2*i)+0.5)*w;
      double w12=(double(2*i+1)+0.5)*w;
      cmds.push_back( str( format("cellSet %s add boxToCell (%g %g %g) (%g %g %g)") 
	% trip_ 
	% (-1e-10)	% /*dtrip*/0.0 	% w0 
	% Ltrip 	% (/*2.**/dtrip) 	% w12 
      ) );
    }
    cmds.push_back( str( format("cellSet %s invert") % trip_ ) );
    
    setSet(cm, executionPath(), cmds);
    
    removeCellSetFromMesh(cm, executionPath(), trip_);
  }
  else if (tp.selection()=="drag")
  {
//     int n=tp().getInt("n");
    double Reh=tp().getDouble("Reh");
//     double CD=tp().getDouble("CD");
    double lbyh=tp().getDouble("lbyh");
    
    double dtrip=Reh*nu/uinf_;
    double Ltrip=lbyh*dtrip; //3.*dtrip;
    
    std::vector<std::string> cmds;
    cmds.push_back( str( format("cellSet %s new boxToCell (-1e-10 0 -1e10) (%g %g 1e10)") 
	% trip_ 
	% Ltrip % dtrip
    ) );
    
    setSet(cm, executionPath(), cmds);
    
    setsToZones(cm, executionPath(), true);
  }

  cm.executeCommand(executionPath(), "renumberMesh", list_of("-overwrite") );
}


void FlatPlateBL::createCase(insight::OpenFOAMCase& cm)
{
  const ParameterSet& p=*parameters_;

  // create local variables from ParameterSet
  PSDBL(p, "geometry", HBydeltae);
  PSDBL(p, "geometry", WBydeltae);
  PSDBL(p, "geometry", L);
  PSDBL(p, "fluid", nu);
  PSDBL(p, "mesh", ypluswall);
  PSDBL(p, "mesh", dxplus);
  PSDBL(p, "mesh", dzplus);
  PSINT(p, "mesh", nh);
  PSINT(p, "run", np);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);
  
  double Umax=FieldData( p.getSubset("inflow").getSubset("umean")).maxValueMag();
  
  std::string regime = p.get<SelectableSubsetParameter>("run/regime").selection();
  if (regime=="steady")
  {
    cm.insert(new simpleFoamNumerics(cm, simpleFoamNumerics::Parameters()
      .set_hasCyclics(true)
      .set_decompositionMethod("hierarchical")
      .set_endTime(end_)
      .set_checkResiduals(false) // don't stop earlier since averaging should be completed
      .set_Uinternal(vec3(Umax,0,0))
      .set_decompWeights(std::make_tuple(2,1,0))
      .set_np(np)
    ));
  } 
  else if (regime=="unsteady")
  {
    cm.insert( new pimpleFoamNumerics(cm, pimpleFoamNumerics::Parameters()
      .set_maxDeltaT(0.25*T_)
      .set_writeControl("adjustableRunTime")
      .set_writeInterval(0.25*T_)
      .set_endTime(end_)
      .set_decompositionMethod("hierarchical")
      .set_deltaT(1e-3)
      .set_hasCyclics(true)
      .set_Uinternal(vec3(Umax,0,0))
      .set_decompWeights(std::make_tuple(2,1,0))
      .set_np(np)
    ) );
  }
  cm.insert(new extendedForces(cm, extendedForces::Parameters()
    .set_patches( list_of<string>("walls") )
  ));
  
  cm.insert(new fieldAveraging(cm, fieldAveraging::Parameters()
    .set_name("zzzaveraging") // shall be last FO in list
    .set_fields(list_of<std::string>("p")("U")("pressureForce")("viscousForce"))
    .set_timeStart(avgStart_)
  ));
  
//   std::vector<arma::mat> plocs;
//   for(size_t i=0; i<sec_locs_.size(); i++)
//   {
//     plocs.push_back(vec3(sec_locs_[i]*L, 0.5*W_, ));
//   }
//   cm.insert(new probes(cm, probes::Parameters()
//     .set_fields( list_of("U")("p") )
//     .set_probeLocations( list_of 
//       vec3()
//     )
//   ));
  
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

  const SelectableSubsetParameter& tp = p.get<SelectableSubsetParameter>("mesh/tripping");
  if (tp.selection()=="drag")
  {
    double CD=tp().getDouble("CD");

    cm.insert(new volumeDrag(cm, volumeDrag::Parameters()
      .set_name(trip_)
      .set_CD(vec3(CD, 0, 0))
    ));
  }
  
  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(nu) ));
  
//   cm.insert(new VelocityInletBC(cm, in_, boundaryDict, VelocityInletBC::Parameters()
//     .set_velocity(FieldData(vec3(uinf_,0,0)))
//     .set_turbulence(uniformIntensityAndLengthScale(0.005, 0.1*H_))
//   ) );
  cm.insert(new TurbulentVelocityInletBC(cm, in_, boundaryDict, TurbulentVelocityInletBC::Parameters(p.getSubset("inflow")) ));
  cm.insert(new PressureOutletBC(cm, out_, boundaryDict, PressureOutletBC::Parameters()
    .set_pressure(0.0)
  ));
  
//  if (patchExists(boundaryDict, approach_)) // make possible to evaluate old cases without approach patch
//    cm.insert(new SimpleBC(cm, approach_, boundaryDict, "symmetryPlane") );
  //  leave approach as wall to produce laminar BL upstream
  
  cm.insert(new SuctionInletBC(cm, top_, boundaryDict, SuctionInletBC::Parameters()
    .set_pressure(0.0)
  ));
  if (p.getBool("mesh/2d"))
    cm.insert(new SimpleBC(cm, cycl_prefix_, boundaryDict, "empty") );
  else
    cm.insert(new CyclicPairBC(cm, cycl_prefix_, boundaryDict) );
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());
  
  insertTurbulenceModel(cm, p.get<SelectionParameter>("fluid/turbulenceModel").selection());
}

void FlatPlateBL::evaluateAtSection
(
  OpenFOAMCase& cm,
  ResultSetPtr results, double x, int i,
  const Interpolator& cfi,
  const std::string& UMeanName,
  const std::string& RFieldName
)
{
  const ParameterSet& p=*parameters_;
  // create local variables from ParameterSet
  PSDBL(p, "geometry", HBydeltae);
  PSDBL(p, "geometry", WBydeltae);
  PSDBL(p, "geometry", L);
//   PSDBL(p, "operation", Re_L);
  PSDBL(p, "fluid", nu);
  PSDBL(p, "mesh", ypluswall);
  PSDBL(p, "mesh", dxplus);
  PSDBL(p, "mesh", dzplus);
  PSINT(p, "mesh", nh);


  double xByL= x/L;
  string title="section__xByL_" + str(format("%04.1f") % xByL);
  replace_all(title, ".", "_");
  
  TabularResult& table = results->get<TabularResult>("tableCoefficients");
  TabularResult::Row& thisctrow = table.appendRow();
  table.setCellByName(thisctrow, "x/L", xByL);
  
//   //estimate delta2
//   double delta2_est = 0.5*FlatPlateBL::cw(uinf_*x/nu)*x;
    
  boost::ptr_vector<sampleOps::set> sets;
  
  double 
    miny=0.99*deltaywall_e_,
    maxy=std::min(delta2e_*10.0, H_-deltaywall_e_);
    
  arma::mat pts=exp(linspace(log(miny), log(maxy), 101))*vec3(0,1,0).t();
  pts.col(0)+=x;
  pts.col(2)+=0.01*W_;
  
  sets.push_back(new sampleOps::linearAveragedPolyLine(sampleOps::linearAveragedPolyLine::Parameters()
    .set_name("radial")
//     .set_start( vec3(x, deltaywall_e_, 0.01*W_))
//     .set_end(   vec3(x, , 0.01*W_))
    .set_points( pts )
    .set_dir1(vec3(1,0,0))
    .set_dir2(vec3(0,0,0.98*W_))
    .set_nd1(1)
    .set_nd2(5)
  ));
  
  sample(cm, executionPath(), 
     list_of<std::string>("p")(UMeanName)(RFieldName)("k")("omega")("epsilon")("nut"),
     sets
  );
  
  sampleOps::ColumnDescription cd;
  arma::mat data =
    sampleOps::findSet<sampleOps::linearAveragedPolyLine>(sets, "radial").readSamples(cm, executionPath(), &cd);
  arma::mat y=data.col(0)+deltaywall_e_;

  double tauw=as_scalar(0.5*cfi(x)*uinf_*uinf_);
  double utau=sqrt(tauw);
  table.setCellByName(thisctrow, "tauw", tauw);
  table.setCellByName(thisctrow, "utau", utau);
  double ypByy=utau/nu;
  arma::mat yplus=y*ypByy;
    
  int c=cd[UMeanName].col;
  arma::mat upaxial(join_rows(yplus, data.col(c)/utau));
  arma::mat upwallnormal(join_rows(yplus, data.col(c+1)/utau));
  arma::mat upspanwise(join_rows(yplus, data.col(c+2)/utau));
  
  arma::mat uByUinf=join_rows(y, data.col(c)/uinf_);
  arma::mat delta123 = integrateDelta123( uByUinf );
  double delta99 = searchDelta99( uByUinf );

  cout<<"delta123="<<delta123<<"delta99="<<delta99<<endl;

  table.setCellByName(thisctrow, "delta1", delta123(0));
  table.setCellByName(thisctrow, "delta2", delta123(1));
  table.setCellByName(thisctrow, "delta3", delta123(2));
  table.setCellByName(thisctrow, "delta99", delta99 );
  table.setCellByName(thisctrow, "delta1+", delta123(0)*ypByy);
  table.setCellByName(thisctrow, "delta2+", delta123(1)*ypByy);
  table.setCellByName(thisctrow, "delta3+", delta123(2)*ypByy);
  table.setCellByName(thisctrow, "delta99+", delta99*ypByy );
  
  // Mean velocity profiles
  {
    {
      arma::mat up=join_rows(yplus, join_rows(upaxial.col(1), join_rows(upwallnormal.col(1), upspanwise.col(1))));
      up.save( (executionPath()/("umeanplus_vs_yplus_"+title+".txt")).c_str(), raw_ascii);
    }
    
    double maxU=1.1*uinf_;
    
    arma::mat delta1pc(delta123(0)*ones(2,2)*ypByy);
    delta1pc(0,1)=0.; delta1pc(1,1)=maxU;
    
    arma::mat delta2pc(delta123(1)*ones(2,2)*ypByy);
    delta2pc(0,1)=0.; delta2pc(1,1)=maxU;

    arma::mat delta3pc(delta123(2)*ones(2,2)*ypByy);
    delta3pc(0,1)=0.; delta3pc(1,1)=maxU;
    
    arma::mat delta99pc(delta99*ones(2,2)*ypByy);
    delta99pc(0,1)=0.; delta99pc(1,1)=maxU;
    
    arma::mat visclayer=linspace(0, 10, 10), loglayer=linspace(30,300,2);
    visclayer=join_rows(visclayer, visclayer);
    loglayer=join_rows(loglayer, (1./0.41)*log(loglayer)+5.);

    addPlot
    (
      results, executionPath(), "chartMeanVelocity_"+title,
      "y+", "<U+>",
      list_of
	(PlotCurve(upaxial, "w l lt 1 lc 1 lw 4 t 'Axial'"))
	(PlotCurve(upspanwise, "w l lt 1 lc 2 lw 4 t 'Spanwise'"))
	(PlotCurve(upwallnormal, "w l lt 1 lc 3 lw 4 t 'Wall normal'"))
	(PlotCurve(delta1pc, "w l lt 2 lc 4 lw 1 t 'delta_1+'"))
	(PlotCurve(delta2pc, "w l lt 3 lc 4 lw 1 t 'delta_2+'"))
	(PlotCurve(delta3pc, "w l lt 4 lc 4 lw 1 t 'delta_3+'"))
	(PlotCurve(delta99pc, "w l lt 5 lc 4 lw 1 t 'delta_99+'"))
	
	(PlotCurve(visclayer, "w l lt 2 lc 5 lw 2 t 'Viscous Layer'"))
	(PlotCurve(loglayer, "w l lt 3 lc 5 lw 2 t 'Log Layer'"))
      ,
      "Wall normal profiles of averaged velocities at x/L=" + str(format("%g")%xByL),
     
      str( format("set key top left reverse Left; set logscale x; set xrange [:%g]; set yrange [0:%g];") 
		% (ypByy*std::max(delta2e_, 10.*delta123(1))) 
		% (maxU/utau) 
	 )
      
    );
  }

  // Reynolds stress profiles
  c=cd[RFieldName].col;
  arma::mat Rpuu(join_rows(yplus, data.col(c)/pow(utau,2)));
  arma::mat Rpvv(join_rows(yplus, data.col(c+3)/pow(utau,2)));
  arma::mat Rpww(join_rows(yplus, data.col(c+5)/pow(utau,2)));
  {
    arma::mat Rp=join_rows(yplus, join_rows(Rpuu.col(1), join_rows(Rpvv.col(1), Rpww.col(1))));
    Rp.save( (executionPath()/("Rplus_vs_yplus_"+title+".txt")).c_str(), raw_ascii);
    
    double maxRp=1.1*as_scalar(arma::max(arma::max(Rp.cols(1,3))));
    
    addPlot
    (
      results, executionPath(), "chartReynoldsStress_"+title,
      "y+", "<R+>",
      list_of
	(PlotCurve(Rpuu, "w l lt 1 lc 1 lw 4 t 'Axial'"))
	(PlotCurve(Rpvv, "w l lt 1 lc 2 lw 4 t 'Wall normal'"))
	(PlotCurve(Rpww, "w l lt 1 lc 3 lw 4 t 'Spanwise'"))
      ,
      "Wall normal profiles of Reynolds stresses at x/L=" + str(format("%g")%xByL),
     
      str( format("set xrange [:%g]; set yrange [0:%g];") 
		% (ypByy*std::max(delta2e_, 10.*delta123(1))) 
		% (maxRp) 
	 )
      
    );
  }
  
  if (cd.find("k")!=cd.end())
  {    
    arma::mat kp_vs_yp=join_rows(ypByy*y, ypByy*data.col(cd["k"].col));
    kp_vs_yp.save( (executionPath()/("kplus_vs_yplus_"+title+".txt")).c_str(), raw_ascii);
    
    addPlot
    (
      results, executionPath(), "chartTKE_"+title,
      "y+", "<k+>",
      list_of
       (PlotCurve(kp_vs_yp, "w l lt 2 lc 1 lw 1 not"))
       ,
      "Wall normal profile of turbulent kinetic energy at x/L=" + str(format("%g")%xByL),
      "set logscale x"
    );
  }
}

insight::ResultSetPtr FlatPlateBL::evaluateResults(insight::OpenFOAMCase& cm)
{
  const ParameterSet& p=*parameters_;
  // create local variables from ParameterSet
  PSDBL(p, "geometry", HBydeltae);
  PSDBL(p, "geometry", WBydeltae);
  PSDBL(p, "geometry", L);
//   PSDBL(p, "operation", Re_L);
  PSDBL(p, "fluid", nu);
  PSINT(p, "mesh", nh);

  ResultSetPtr results = OpenFOAMAnalysis::evaluateResults(cm);
  
  std::string RFieldName="UPrime2Mean";
  std::string UMeanName="UMean";
  if ( const RASModel *rm = cm.get<RASModel>(".*") )
  {
    std::cout<<"Case included RASModel "<<rm->name()<<". Computing R field"<<std::endl;
    cm.executeCommand( executionPath(), "R", list_of("-latestTime") );
    RFieldName="R";
    UMeanName="U";
  }
  
  {
    cm.executeCommand(executionPath(), "Lambda2", list_of("-latestTime"));
    
    double lambda2=1e5;
    
    std::string name="figVortexStructures";
    std::string fname=name+".png";
    runPvPython
    (
      cm, executionPath(), list_of<std::string>
      (
	"cbi=loadOFCase('.')\n"
	+str(format("L=%g\n")%L)+
	"prepareSnapshots()\n"

	"eb=extractPatches(cbi, '^(wall|approach|oldInternalFaces)')\n"
	"Show(eb)\n"
	"displayContour(eb, 'viscousForce', arrayType='CELL_DATA', barpos=[0.75,0.15], barorient=0)\n"

	"interior=extractInterior(cbi)\n"
	"contour1 = Contour(Input=interior)\n"
	"contour1.PointMergeMethod = 'Uniform Binning'\n"
	"contour1.ContourBy = ['POINTS', 'Lambda2']\n"
	+str(format("contour1.Isosurfaces = [%g]\n")%lambda2)+
	"displaySolid(contour1)\n"

	"setCam([-L,0.15*L,0.15*L], [0.5*L,0,0], [0,1,0], 0.1*L)\n"
	"WriteImage('"+fname+"')\n"
      )
    );
    results->insert(name,
      std::auto_ptr<Image>(new Image
      (
      executionPath(), fname, 
      str(format("Vortex structures, vizualized as isosurfaces of $\\lambda_2=%g$")%(-lambda2)), ""
    )));  
  }

  // Wall friction coefficient
  arma::mat wallforce=viscousForceProfile(cm, executionPath(), vec3(1,0,0), nax_);
    
  arma::mat Cf_vs_x(join_rows(
      wallforce.col(0), 
      wallforce.col(1)/(0.5*pow(uinf_,2))
    ));
  Cf_vs_x.save( (executionPath()/"Cf_vs_x.txt").c_str(), arma_ascii);
  
  Interpolator Cf_vs_x_i(Cf_vs_x);

  {  
    
    arma::mat Cfexp_vs_x=refdatalib.getProfile("Wieghardt1951_FlatPlate", "u17.8/cf_vs_x");

    addPlot
    (
      results, executionPath(), "chartMeanWallFriction",
      "x [m]", "<Cf>",
      list_of
	(PlotCurve(Cf_vs_x, "w l lt 1 lc 2 lw 2 t 'CFD'"))
	(PlotCurve(Cfexp_vs_x, "w p lt 2 lc 2 t 'Wieghardt 1951 (u=17.8m/s)'"))
	,
      "Axial profile of wall friction coefficient"
    );    
  }
  
  results->insert("tableCoefficients",
    std::auto_ptr<TabularResult>(new TabularResult
    (
      list_of("x/L")("delta1")("delta1+")("delta2")("delta2+")("delta3")("delta3+")("delta99")("delta99+")("tauw")("utau"),
      arma::mat(),
      "Boundary layer properties along the plate", "", ""
  )));
  
  for (size_t i=0; i<sec_locs_.size(); i++)
    evaluateAtSection(cm, results, sec_locs_[i]*L, i, Cf_vs_x_i, UMeanName, RFieldName);

  {  
    arma::mat delta1exp_vs_x=refdatalib.getProfile("Wieghardt1951_FlatPlate", "u17.8/delta1_vs_x");
    arma::mat delta2exp_vs_x=refdatalib.getProfile("Wieghardt1951_FlatPlate", "u17.8/delta2_vs_x");
    arma::mat delta3exp_vs_x=refdatalib.getProfile("Wieghardt1951_FlatPlate", "u17.8/delta3_vs_x");
    
    const insight::TabularResult& tabres=results->get<TabularResult>("tableCoefficients");
    arma::mat ctd=tabres.toMat();
    addPlot
    (
      results, executionPath(), "chartDelta",
      "x [m]", "delta [m]",
      list_of
	(PlotCurve(delta1exp_vs_x, "w p lt 1 lc 1 t 'delta_1 (Wieghardt 1951, u=17.8m/s)'"))
	(PlotCurve(delta2exp_vs_x, "w p lt 2 lc 3 t 'delta_2 (Wieghardt 1951, u=17.8m/s)'"))
	(PlotCurve(delta3exp_vs_x, "w p lt 3 lc 4 t 'delta_3 (Wieghardt 1951, u=17.8m/s)'"))
	
	(PlotCurve(arma::mat(join_rows(L*ctd.col(0), tabres.getColByName("delta1"))), "w l lt 1 lc 1 lw 2 t 'delta_1'"))
	(PlotCurve(arma::mat(join_rows(L*ctd.col(0), tabres.getColByName("delta2"))), "w l lt 1 lc 3 lw 2 t 'delta_2'"))
	(PlotCurve(arma::mat(join_rows(L*ctd.col(0), tabres.getColByName("delta3"))), "w l lt 1 lc 4 lw 2 t 'delta_3'"))
	,
      "Axial profile of boundary layer thickness",
      "set key top left reverse Left"
    );

    addPlot
    (
      results, executionPath(), "chartDelta99",
      "x [m]", "delta [m]",
      list_of
	(PlotCurve(arma::mat(join_rows(L*ctd.col(0), tabres.getColByName("delta1"))), "w l lt 1 lc 1 lw 2 t 'delta_1'"))
	(PlotCurve(arma::mat(join_rows(L*ctd.col(0), tabres.getColByName("delta2"))), "w l lt 1 lc 3 lw 2 t 'delta_2'"))
	(PlotCurve(arma::mat(join_rows(L*ctd.col(0), tabres.getColByName("delta3"))), "w l lt 1 lc 4 lw 2 t 'delta_3'"))
	(PlotCurve(arma::mat(join_rows(L*ctd.col(0), tabres.getColByName("delta99"))), "w l lt 1 lc 5 lw 2 t 'delta_99'"))
	,
      "Axial profile of boundary layer thickness",
      "set key top left reverse Left"
    );

  }
  
  
  
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

arma::mat FlatPlateBL::integrateDelta123(const arma::mat& uByUinf_vs_y)
{
  arma::mat delta(zeros(3));
  
  arma::mat x = uByUinf_vs_y.col(0);
  arma::mat y = uByUinf_vs_y.col(1);
  for (int i=0; i<y.n_elem; i++) y(i)=std::max(0., std::min(1., y(i)));
  //arma::mat y = clamp(uByUinf_vs_y.col(1), 0., 1);

  arma::mat y1, y2, y3;
  y1 = (1. - y);
  y2 = y % (1. - y);
  y3 = y % (1. - pow(y,2));
  
  if (fabs(x(0)) > 1e-10)
  {
    delta(0) += 0.5*y1(0) * x(0);
    delta(1) += 0.5*y2(0) * x(0);
    delta(2) += 0.5*y3(0) * x(0);
  }
  
  for (int i=0; i<uByUinf_vs_y.n_rows-1; i++)
  {
    delta(0) += 0.5*( y1(i) + y1(i+1) ) * ( x(i+1) - x(i) );
    delta(1) += 0.5*( y2(i) + y2(i+1) ) * ( x(i+1) - x(i) );
    delta(2) += 0.5*( y3(i) + y3(i+1) ) * ( x(i+1) - x(i) );
  }
  
  return delta;
}

double FlatPlateBL::searchDelta99(const arma::mat& uByUinf_vs_y)
{
  int i=0;
  for (i=0; i<uByUinf_vs_y.n_rows; i++)
  {
    if (uByUinf_vs_y(i,1)>=0.99) break;
  }
  return uByUinf_vs_y(std::min(i, int(uByUinf_vs_y.n_rows-1)),0);
}


}
