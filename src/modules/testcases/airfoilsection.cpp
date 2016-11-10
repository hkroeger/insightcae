/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
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
 */

#include "base/factory.h"
#include "airfoilsection.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/snappyhexmesh.h"

#include "boost/foreach.hpp"
#include "boost/assign.hpp"
#include <boost/assign/ptr_map_inserter.hpp>
#include <boost/assign/ptr_list_of.hpp>

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{
 
  
  
  
defineType(AirfoilSection);
addToFactoryTable(Analysis, AirfoilSection);




AirfoilSection::AirfoilSection()
: OpenFOAMAnalysis("Airfoil 2D", "Steady RANS simulation of a 2-D flow over an airfoil section"),
  in_("in"), 
  out_("out"), 
  up_("up"), 
  down_("down"), 
  fb_("frontAndBack"),
  foil_("foil")
{

}




insight::ParameterSet AirfoilSection::defaultParameters() const
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
	    ("c",		new DoubleParameter(1.0, "[m] chord length"))
	    ("alpha",		new DoubleParameter(0.0, "[deg] angle of attack"))
	    ("LinByc",		new DoubleParameter(1.0, "[-] Upstream extent of the channel"))
	    ("LoutByc",		new DoubleParameter(4.0, "[-] Downstream extent of the channel"))
	    ("HByc",		new DoubleParameter(2.0, "[-] Height of the channel"))
	    ("foilfile",	new PathParameter("foil.csv", "File with tabulated coordinates on the foil surface"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Geometrical properties of the numerical tunnel"
	))
      
      ("mesh", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("nc",	new IntParameter(15, "# cells along span"))
	    ("lmfoil",	new IntParameter(5, "min refinement level at foil surface"))
	    ("lxfoil",	new IntParameter(6, "min refinement level at foil surface"))
	    ("nlayer",	new IntParameter(10, "number of prism layers"))
// 	    ("ypluswall", new DoubleParameter(2, "yPlus at the wall grid layer"))
// 	    ("2d",	new BoolParameter(false, "Whether to create a two-dimensional case"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Properties of the computational mesh"
	))
      
      ("operation", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("vinf",		new DoubleParameter(30.8, "[m/s] inflow velocity"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Definition of the operation point under consideration"
	))
      
      ("fluid", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("rho",	new DoubleParameter(1.0, "[kg/m^3] Density of the fluid"))
	    ("nu",	new DoubleParameter(1.5e-5, "[m^2/s] Viscosity of the fluid"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Parameters of the fluid"
	))
      
      //       ("evaluation", new SubsetParameter
// 	(
// 	  ParameterSet
// 	  (
// 	    boost::assign::list_of<ParameterSet::SingleEntry>
// 	    ("inittime",	new DoubleParameter(5, "[T] length of grace period before averaging starts (as multiple of flow-through time)"))
// 	    ("meantime",	new DoubleParameter(10, "[T] length of time period for averaging of velocity and RMS (as multiple of flow-through time)"))
// 	    ("mean2time",	new DoubleParameter(10, "[T] length of time period for averaging of second order statistics (as multiple of flow-through time)"))
// 	    ("eval2", 		new BoolParameter(true, "Whether to evaluate second order statistics"))
// 	    .convert_to_container<ParameterSet::EntryList>()
// 	  ), 
// 	  "Options for statistical evaluation"
// 	))
      
      .convert_to_container<ParameterSet::EntryList>()
  );
  
  return p;
}




void AirfoilSection::createMesh(insight::OpenFOAMCase& cm)
{
  const ParameterSet& p = *parameters_;

  PSPATH(p, "geometry", foilfile);
  PSINT(p, "mesh", nlayer);
  PSINT(p, "mesh", lmfoil);
  PSINT(p, "mesh", lxfoil);
  PSINT(p, "mesh", nc);
  PSDBL(p, "geometry", c);
  PSDBL(p, "geometry", alpha);
  PSDBL(p, "geometry", LinByc);
  PSDBL(p, "geometry", LoutByc);
  PSDBL(p, "geometry", HByc);
  
  arma::mat contour;
  contour.load(foilfile.string(), arma::auto_detect);
  cout<<contour<<endl;
  
  path dir = executionPath();
    
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "wall");
  
  double delta=c/double(nc);
  
  std::map<int, Point> pts;
  double z0=0, h=delta;
  pts = boost::assign::map_list_of   
      (0, 	vec3(-(LinByc+0.5)*c, -HByc*c, z0))
      (1, 	vec3((LoutByc+0.5)*c, -HByc*c, z0))
      (2, 	vec3((LoutByc+0.5)*c, HByc*c, z0))
      (3, 	vec3(-(LinByc+0.5)*c, HByc*c, z0))
      .convert_to_container<std::map<int, Point> >()
  ;
  
  arma::mat PiM=vec3(-(LinByc+0.4)*c, 0.01*c, z0+0.0001*h);
  
  int nx=(pts[1][0]-pts[0][0])/delta;
  int ny=(pts[2][1]-pts[1][1])/delta;
  
  Patch& in= 	bmd->addPatch(in_, new Patch());
  Patch& out= 	bmd->addPatch(out_, new Patch());
  Patch& up= 	bmd->addPatch(up_, new Patch());
  Patch& down= 	bmd->addPatch(down_, new Patch());
  Patch& dummy= bmd->addPatch("dummy", new Patch());
  Patch& fb= 	bmd->addPatch(fb_, new Patch());
  
  arma::mat vH=vec3(0,0,h);
  
#define PTS(a,b,c,d) \
  P_8(pts[a], pts[b], pts[c], pts[d], \
      pts[a]+vH, pts[b]+vH, pts[c]+vH, pts[d]+vH)
      
  {
    Block& bl = bmd->addBlock
    (  
      new Block(PTS(0,1,2,3),
	nx, ny, 1,
	list_of<double>(1.)(1.)(1.)
      )
    );
    in.addFace(bl.face("0473"));
    out.addFace(bl.face("1265"));
    up.addFace(bl.face("2376"));
    down.addFace(bl.face("0154"));
    fb.addFace(bl.face("0321"));
    dummy.addFace(bl.face("4567"));
  }
  
  cm.insert(bmd.release());

  cm.createOnDisk(dir);
  
  path targ_path(dir/"constant"/"triSurface"/"foil.stl");
  create_directories(targ_path.parent_path());
  STLExtruder(contour, 0, z0+2.0, targ_path);
  
  cm.executeCommand(dir, "blockMesh");  

  boost::ptr_vector<snappyHexMeshFeats::Feature> shm_feats;
  
  shm_feats.push_back(new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
    .set_name(foil_)
    .set_fileName(targ_path)
    .set_minLevel(lmfoil)
    .set_maxLevel(lxfoil)
    .set_nLayers(nlayer)
    .set_scale(vec3(c, c, 1))
    .set_rollPitchYaw(vec3(0,0,-alpha))
  ));
  
  shm_feats.push_back(new snappyHexMeshFeats::NearSurfaceRefinement( snappyHexMeshFeats::NearSurfaceRefinement::Parameters()
    .set_name(foil_)
    .set_mode("distance")
    .set_level(lmfoil)
    .set_distance(0.1*c)
  ));

    
  snappyHexMesh
  (
    cm, dir,
    OFDictData::vector3(PiM),
    shm_feats,
    snappyHexMeshOpts::Parameters()
      .set_tlayer(0.25)
      .set_relativeSizes(true)
      .set_nLayerIter(10)
  );
  
  extrude2DMesh
  (
    cm, dir,
    fb_
  );
}




void AirfoilSection::createCase(insight::OpenFOAMCase& cm)
{
  // create local variables from ParameterSet
  PSDBL(parameters(), "geometry", c);
  PSDBL(parameters(), "geometry", alpha);
  PSDBL(parameters(), "operation", vinf);
  PSINT(parameters(), "fluid", turbulenceModel);
  PSDBL(parameters(), "fluid", nu);
  PSDBL(parameters(), "fluid", rho);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

  cm.insert(new simpleFoamNumerics(cm, simpleFoamNumerics::Parameters()
    .set_purgeWrite(2)
    .set_endTime(5000)
  )); 
  
  cm.insert(new forces(cm, forces::Parameters()
    .set_name("foilForces")
    .set_patches( list_of("\""+foil_+".*\"") )
    .set_rhoInf(rho)
    .set_CofR(vec3(0,0,0))
    ));  

//   cm.insert(new minMaxSurfacePressure(cm, minMaxSurfacePressure::Parameters()
//       .set_name("minPressure")
//       .set_patches( list_of("\""+foil_+".*\"") )
//       .set_nblades(1)
//       .set_section_radii(list_of(0.0)(1.0))
//       ));

  cm.insert(new VelocityInletBC(cm, in_, boundaryDict, VelocityInletBC::Parameters().velocity(vec3(vinf,0,0)) ));
  cm.insert(new PressureOutletBC(cm, out_, boundaryDict, PressureOutletBC::Parameters().set_pressure(0.0) ));
   
  cm.insert(new SimpleBC(cm, up_, boundaryDict, "symmetryPlane" ));
  cm.insert(new SimpleBC(cm, down_, boundaryDict, "symmetryPlane" ));
  cm.insert(new SimpleBC(cm, fb_, boundaryDict, "empty" ));

  //   cm.insert(new cuttingPlane(cm, cuttingPlane::Parameters()
//     .set_name("plane")
//     .set_basePoint(vec3(0,1e-6,1e-6))
//     .set_normal(vec3(0,0,1))
//     .set_fields(list_of<string>("p")("U")("UMean")("UPrime2Mean"))
//   ));
  
//   cm.insert(new fieldAveraging(cm, fieldAveraging::Parameters()
//     .set_name("zzzaveraging") // shall be last FO in list
//     .set_fields(list_of<std::string>("p")("U"))
//     .set_timeStart(inittime*T_)
//   ));
  
//   cm.insert(new RadialTPCArray(cm, typename RadialTPCArray::Parameters()
//     .set_name_prefix("tpc_interior")
//     .set_R(0.5*D)
//     .set_x(0.5*L)
//     .set_axSpan(0.5*L)
//     .set_tanSpan(M_PI)
//     .set_timeStart( (inittime+meantime)*T_ )
//   ));
//   
  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(nu) ));
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());
  insertTurbulenceModel(cm, parameters().get<SelectionParameter>("fluid/turbulenceModel").selection());
}




insight::ResultSetPtr AirfoilSection::evaluateResults(insight::OpenFOAMCase& cm)
{
  const ParameterSet& p = *parameters_;
  PSDBL(p, "geometry", c);
  PSDBL(p, "geometry", alpha);
  PSDBL(p, "operation", vinf);
  PSDBL(p, "fluid", nu);
  PSDBL(p, "fluid", rho);
  
  ResultSetPtr results = OpenFOAMAnalysis::evaluateResults(cm);
  
  arma::mat f_vs_iter=forces::readForces(cm, executionPath(), "foilForces");
  
  double Aref=1.*c, Re=c*vinf/nu;
  
  ptr_map_insert<ScalarResult>(*results) 
    ("Aref", Aref, "Reference area", "", "$m^2$");
  ptr_map_insert<ScalarResult>(*results) 
    ("Re", Re, "Reynolds number", "", "");
  
  arma::mat cl = (f_vs_iter.col(2)+f_vs_iter.col(5))
		  / ( 0.5*rho * pow(vinf,2) * Aref );
  arma::mat cd = (f_vs_iter.col(1)+f_vs_iter.col(4))
		  / ( 0.5*rho * pow(vinf,2) * Aref );
  arma::mat eps = cl/cd;
  
  double minPbyrho=minPatchPressure(cm, executionPath(), "foil")(0,1);
  double cpmin=minPbyrho/(0.5*pow(vinf,2));
  
  ptr_map_insert<ScalarResult>(*results) 
    ("cl", cl(cl.n_elem-1), "Lift coefficient", "", "");
  ptr_map_insert<ScalarResult>(*results) 
    ("cd", cd(cd.n_elem-1), "Drag coefficient", "", "");
  ptr_map_insert<ScalarResult>(*results) 
    ("eps", eps(eps.n_elem-1), "Lift-to-drag ratio", "", "");
  ptr_map_insert<ScalarResult>(*results) 
    ("cpmin", cpmin, "Minimum pressure", "", "");
    
  addPlot
  (
    results, executionPath(), "chartCoefficientConvergence",
    "Iteration", "$C_L$, $C_D$",
    list_of
     (PlotCurve( arma::mat(join_rows(f_vs_iter.col(0), cl)), "CL", "w l t '$C_L$'" ))
     (PlotCurve( arma::mat(join_rows(f_vs_iter.col(0), cd)), "CD", "w l t '$C_D$'" ))
     (PlotCurve( arma::mat(join_rows(f_vs_iter.col(0), eps)), "CLbyCD", "axes x1y2 w l t '$C_L/C_D$'" ))
    ,
    "Convergence history of coefficients",
    "set y2tics;set y2label 'C_L/C_D'"
  );
  
  std::string init=
      "cbi=loadOFCase('.')\n"
      "prepareSnapshots()\n";
    
  std::string figname("foilflow"), fname(figname+".png");
  runPvPython
  (
    cm, executionPath(), list_of<std::string>
    (
      init+
"interior=extractInterior(cbi)\n"
"# create a new 'Slice'\n"
"slice1 = Slice(Input=interior)\n"
"slice1.SliceType = 'Plane'\n"
"slice1.SliceOffsetValues = [0.0]\n"
"# Properties modified on slice1.SliceType\n"
"slice1.SliceType.Origin = [0, 0, 0]\n"
"slice1.SliceType.Normal = [0, 0, 1]\n"

"surfaceVectors1 = SurfaceVectors(Input=slice1)\n"
"surfaceVectors1.SelectInputVectors = ['POINTS', 'U']\n"
"# create a new 'Mask Points'\n"

"maskPoints1 = MaskPoints(Input=surfaceVectors1)\n"
"# Properties modified on maskPoints1\n"
"maskPoints1.MaximumNumberofPoints = 100\n"
"maskPoints1.ProportionallyDistributeMaximumNumberOfPoints = 1\n"
"maskPoints1.RandomSampling = 1\n"
"# create a new 'Stream Tracer With Custom Source'\n"
"streamTracerWithCustomSource1 = StreamTracerWithCustomSource(Input=surfaceVectors1,\n"
" SeedSource=maskPoints1)\n"
"streamTracerWithCustomSource1.Vectors = ['POINTS', 'U']\n"
"streamTracerWithCustomSource1.MaximumStreamlineLength = 100\n"

"displayContour(slice1, 'p', arrayType='CELL_DATA', barpos=[0.8,0.25], barorient=1)\n"
"Show(streamTracerWithCustomSource1)\n"
"streamTracerWithCustomSource1Display = GetDisplayProperties(streamTracerWithCustomSource1, view=GetActiveView())\n"
"ColorBy(streamTracerWithCustomSource1Display, None)\n"
"setCam(["+lexical_cast<std::string>(0.5*c)+",0,1], ["+lexical_cast<std::string>(0.5*c)+",0,0], [0,1,0], "+lexical_cast<std::string>(c)+")\n"
  
	"WriteImage('"+fname+"')\n"
    )
  );

  results->insert(figname,
    std::auto_ptr<Image>(new Image
    (
    executionPath(), fname, 
    str(format("Relative velocity (angle of attack %gdeg)")%alpha), ""
  )));  
  
  return results;
}


defineType(AirfoilSectionPolar);
addToFactoryTable(Analysis, AirfoilSectionPolar);

AirfoilSectionPolar::AirfoilSectionPolar()
: OpenFOAMParameterStudy
  (
    "Polar of Airfoil",
    "Computes the polar of a 2D airfoil section using CFD",
    AirfoilSection(),
    list_of<std::string>("geometry/alpha"),
    true
  )
{
}

void AirfoilSectionPolar::evaluateCombinedResults(ResultSetPtr& results)
{

  std::string key="coeffTable";
  results->insert(key, table("", "", "geometry/alpha", 
			     list_of<std::string>("cl")("cd")("eps")("cpmin")));
  const TabularResult& tab = 
    static_cast<const TabularResult&>(*(results->find(key)->second));
  
  arma::mat tabdat=tab.toMat();
  
  int order=min(int(tabdat.n_rows)-1, 5);
  arma::mat cl_coeffs=polynomialRegression(tabdat.col(1), tabdat.col(0), order);
  arma::mat cd_coeffs=polynomialRegression(tabdat.col(2), tabdat.col(0), order);
  arma::mat cpmin_coeffs=polynomialRegression(tabdat.col(4), tabdat.col(0), order);
  
  cout<<"Regression cl: "<<cl_coeffs<<endl;
  cout<<"Regression cd: "<<cd_coeffs<<endl;
  cout<<"Regression cpmin: "<<cpmin_coeffs<<endl;
  
  results->insert("RegressionCl", polynomialFitResult(cl_coeffs, "alpha", "Regression Coefficients for cl", ""));
  results->insert("RegressionCd", polynomialFitResult(cd_coeffs, "alpha", "Regression Coefficients for cd", ""));
  results->insert("RegressionCpmin", polynomialFitResult(cpmin_coeffs, "alpha", "Regression Coefficients for cpmin", ""));

//   PropellerCurves owc(arma::flipud(cl_coeffs), arma::flipud(cd_coeffs), arma::flipud(cpmin_coeffs));
//   owc.saveToFile( executionPath()/"airfoilcurves.ist" );
  
  arma::mat ralpha = arma::linspace(0.0 /*min(tabdat.col(0))*/, max(tabdat.col(0)), 50);
  arma::mat raa; raa.resize(ralpha.n_rows, order);
  for (int j=0; j<order; j++) raa.col(j)=pow(ralpha, j);
  
  arma::mat rcl = raa * cl_coeffs;
  arma::mat rcd = raa * cd_coeffs;
  arma::mat rcpmin = raa * cpmin_coeffs;
  
//   std::cout<<ralpha<<"; "<<rcl<<"; "<<rcd<<std::endl;
//   arma::mat reta=arma::mat(join_rows(ralpha, (rJ%rKt)/(2.*M_PI*0.1*rKq10)));
//   reta = arma::mat(reta.rows( find(rKt>0) ));
//   double maxJ=max(reta.col(0)); // maximum J, where thrust is positive
  
  addPlot
  (
    results, executionPath(), "chartAirfoilCharacteristics",
    "$\\alpha$ / deg", "$C_L$, $C_D$",
    list_of<PlotCurve>
     (PlotCurve(arma::mat(join_rows(tabdat.col(0), tabdat.col(1))), 	"CL", "w p lt 1 lc 1 lw 2 t '$C_L$'"))
     (PlotCurve(arma::mat(join_rows(tabdat.col(0), tabdat.col(2))), 	"CD", "w p lt 1 lc 2 lw 2 t '$C_D$'"))
     (PlotCurve(arma::mat(join_rows(ralpha, rcl)), 	"CLfit", "w l lt 1 lc 1 lw 2 t '$C_L$ (regr.)'"))
     (PlotCurve(arma::mat(join_rows(ralpha, rcd)), 	"CDfit", "w l lt 1 lc 2 lw 2 t '$C_D$ (regr.)'"))
     (PlotCurve(arma::mat(join_rows(ralpha, rcpmin)), 	"Cpfit", "w l lt 2 lc 1 lw 1 axes x1y2 t '$C_{p,min}$ (regr.)'"))
    ,
    "Characteristic chart of propeller coefficients",
    "set y2tics; set y2label '$C_P$';"
  );

  
  addPlot
  (
    results, executionPath(), "chartPolar",
    "$C_D$", "$C_L$",
    list_of
     (PlotCurve( arma::mat(join_rows(tabdat.col(2), tabdat.col(1))), "polar", "w l not" ))
    ,
    "Profile polar"
  );

  addPlot
  (
    results, executionPath(), "chartSigma",
    "$C_L$", "$\\sigma$",
    list_of
     (PlotCurve( arma::mat(join_rows(tabdat.col(1), tabdat.col(4))), "sigma", "w l not" ))
    ,
    "Minimum pressure vs. lift coefficient"
  );

//   std::vector<PlotCurve> curves;
//   int i=0;
//   BOOST_FOREACH( const AnalysisInstance& ai, queue_.processed() )
//   {
//     const std::string& n = get<0>(ai);
//     const PropellerAnalysis& a = dynamic_cast<PropellerAnalysis&>(*get<1>(ai));
//     const ResultSetPtr& r = get<2>(ai);
//     
//     curves.push_back(PlotCurve(a.sigmaVsX(a.p()), str( format(" w l lt 1 lc %d t '%s'") % i % n ) ));
//     
//     i++;
//   }
//   
//   addPlot
//   (
//     results, executionPath(), "chartSigmaVsX",
//     "x", "sigma",
//     curves,
//     "Cavitation number vs. radius"
//   );
}




}
