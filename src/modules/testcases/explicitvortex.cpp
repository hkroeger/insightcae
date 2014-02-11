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

#include "explicitvortex.h"

#include "base/factory.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/basiccaseelements.h"

#include <boost/assign/list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"

using namespace arma;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{
  
defineType(ExplicitVortex);

ExplicitVortex::ExplicitVortex(const NoParameters&)
: OpenFOAMAnalysis
  (
    "Explicit Vortex",
    "Rectangular domain with dirichlet BCs for velocity everywhere describing a vortex"
  )
{}

ExplicitVortex::~ExplicitVortex()
{

}

ParameterSet ExplicitVortex::defaultParameters() const
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
	    ("Lx",		new DoubleParameter(1.0, "[m] Extension in X direction"))
	    ("Ly",		new DoubleParameter(1.0, "[m] Extension in Y direction"))
	    ("Lz",		new DoubleParameter(1.0, "[m] Extension in Z direction"))
	    ("vcx",		new DoubleParameter(0.0, "[m] X coord of vortex center"))
	    ("vcy",		new DoubleParameter(-0.3, "[m] Y coord of vortex center"))
	    ("b", 		new DoubleParameter(100, "[mm] Width of bearing shell"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Geometrical properties of the bearing"
	))
      
      ("mesh", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("nx",	new IntParameter(20, "# cells in X"))
	    ("ny",	new IntParameter(20, "# cells in Y"))
	    ("nz",	new IntParameter(20, "# cells in Z"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Properties of the computational mesh"
	))
      
      ("operation", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("Gamma",		new DoubleParameter(0.01, "[-] Vortex strength"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Definition of the operation point under consideration"
	))
      
      ("fluid", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("nu",	new DoubleParameter(0.0, "[m^2/s] Viscosity of the fluid"))
	    ("RASModel",new SelectionParameter(0, 
					       boost::assign::list_of<std::string>
					       ("laminar")
					       ("kOmegaSST")
					       ("kEpsilon")
					       ("kOmegaSST_LowRe")
					       ("kOmegaSST2")
					       .convert_to_container<SelectionParameter::ItemList>(),
					       "Turbulence model"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Parameters of the fluid"
	))
      
      .convert_to_container<ParameterSet::EntryList>()
  );
  
  return p;
}

void ExplicitVortex::cancel()
{
  stopFlag_=true;
}

void ExplicitVortex::createMesh
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{  
  // create local variables from ParameterSet
  path dir = executionPath();
  
  PSDBL(p, "geometry", Lx);
  PSDBL(p, "geometry", Ly);
  PSDBL(p, "geometry", Lz);
  PSDBL(p, "geometry", vcx);
  PSDBL(p, "geometry", vcy);

  PSINT(p, "mesh", nx);
  PSINT(p, "mesh", ny);
  PSINT(p, "mesh", nz);
  
  PSDBL(p, "operation", Gamma);
  
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("symmetryPlanes", "symmetryPlane");
  
  std::map<int, Point> pts;
  
  pts = boost::assign::map_list_of 
    
      (0, 	vec3(-Lx/2., -Ly/2., -Lz/2.))
      (1, 	vec3( Lx/2., -Ly/2., -Lz/2.))
      (2, 	vec3( Lx/2.,  Ly/2., -Lz/2.))
      (3, 	vec3(-Lx/2.,  Ly/2., -Lz/2.))
      (4, 	vec3(-Lx/2., -Ly/2.,  Lz/2.))
      (5, 	vec3( Lx/2., -Ly/2.,  Lz/2.))
      (6, 	vec3( Lx/2.,  Ly/2.,  Lz/2.))
      (7, 	vec3(-Lx/2.,  Ly/2.,  Lz/2.))
  ;

  
  // create patches
  Patch& sides = 	bmd->addPatch("sides", new Patch());
  
  {
    std::map<int, Point>& p = pts;
    
    {
      Block& bl = bmd->addBlock
      (
	new Block(P_8(
	    p[0], p[1], p[2], p[3],
	    p[4], p[5], p[6], p[7]
	  ),
	  nx, ny, nz
	)
      );
      sides.addFace(bl.face("0154"));
      sides.addFace(bl.face("1265"));
      sides.addFace(bl.face("2376"));
      sides.addFace(bl.face("0473"));
    }
  }
  
  cm.insert(bmd.release());

  cm.createOnDisk(dir);
  cm.executeCommand(dir, "blockMesh");  
}

void ExplicitVortex::createCase
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{
  // create local variables from ParameterSet
  PSDBL(p, "geometry", vcx);
  PSDBL(p, "geometry", vcy);
  PSDBL(p, "operation", Gamma);
  PSDBL(p, "fluid", nu);
  PSINT(p, "fluid", RASModel);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);
    
  cm.insert(new simpleFoamNumerics(cm));
  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(nu) ));
  
  cm.insert(new SimpleBC(cm, "symmetryPlanes", boundaryDict, "symmetryPlane"));
  cm.insert(new VelocityInletBC(cm, "sides", boundaryDict, VelocityInletBC::Parameters() ));
  
  insertTurbulenceModel(cm, p.get<SelectionParameter>("fluid/RASModel").selection());
  
  //cm.createOnDisk(dir);
  boost::shared_ptr<OFdicts> dicts=cm.createDictionaries();
  
  cm.createOnDisk(dir, dicts);

  cm.executeCommand(dir, "setVortexVelocity",
    list_of<std::string>
    (lexical_cast<std::string>(Gamma))
    ("("+lexical_cast<std::string>(vcx)+" "+lexical_cast<std::string>(vcy)+" 0)")
  );  
}

ResultSetPtr ExplicitVortex::operator()(ProgressDisplayer* displayer)
{
  const ParameterSet& p = *parameters_;
  
  PSDBL(p, "geometry", Lx);
  PSDBL(p, "geometry", Ly);
  PSDBL(p, "geometry", Lz);

  PSSTR(p, "run", machine);
  
  OFEnvironment ofe(220, "/home/hannes/OpenFOAM/bashrc.of22x");
  ofe.setExecutionMachine(machine);
  
  path dir = setupExecutionEnvironment();

  p.saveToFile(dir/"parameters.ist", type());
  
  {
    OpenFOAMCase meshCase(ofe);
    if (!meshCase.meshPresentOnDisk(dir))
      createMesh(meshCase, p);
    else
      cout<<"case in "<<dir<<": mesh is already there, skipping mesh creation."<<endl;
  }

  OpenFOAMCase runCase(ofe);
  if (!runCase.outputTimesPresentOnDisk(dir))
  {
    createCase(runCase, p);
  }
  else
    cout<<"case in "<<dir<<": output timestep are already there, skipping case recreation and run."<<endl;    
  
  SolverOutputAnalyzer analyzer(*displayer);
  runCase.runSolver(dir, analyzer, "simpleFoam", &stopFlag_);
/*  
  double pclip = (pcav-pambient)/rho - 9.81*depthByD*Di;
  BearingForceList bfl = calcBearingForce(runCase, Di, pclip, dir);
  
  const PatchBearingForce& bf = bfl.find("pivot")->second;
  const arma::mat& f = get<0>(bf);
  const arma::mat& m = get<1>(bf);
  double minp = get<2>(bf);
  
  double fac=1.0; if (symm) fac=2.0;
  */
  ResultSetPtr results(new ResultSet(p, "Circular Journal Bearing Analysis", "Result Report"));
  
  std::string init=
      "cbi=loadOFCase('.')\n"
      "eb=extractPatches(cbi, 'symmetryPlanes')\n"
      "Show(eb)\n"
      "prepareSnapshots()\n";
      
  runPvPython
  (
    runCase, dir, list_of<std::string>
    (
      init+
      "displayContour(eb, 'p', arrayType='CELL_DATA')\n"
      "setCam([0,0,"+lexical_cast<std::string>(Lz)+"], [0,0,0], [0,1,0])\n"
      "WriteImage('pressure_above.jpg')\n"
    )
  );
  results->insert("pressureContour",
    std::auto_ptr<Image>(new Image
    (
    "pressure_above.jpg", 
    "Contour of pressure", ""
  )));
  
  runPvPython
  (
    runCase, dir, list_of<std::string>
    (
      init+
      "displayContour(eb, 'U', component=0, arrayType='CELL_DATA')\n"
      "setCam([0,0,"+lexical_cast<std::string>(Lz)+"], [0,0,0], [0,1,0])\n"
      "WriteImage('Ux_above.jpg')\n"
    )
  );
  results->insert("UxContour",
    std::auto_ptr<Image>(new Image
    (
    "Ux_above.jpg", 
    "Contour of X-Velocity", ""
  )));
  
  runPvPython
  (
    runCase, dir, list_of<std::string>
    (
      init+
      "displayContour(eb, 'U', component=1)\n"
      "setCam([0,0,"+lexical_cast<std::string>(Lz)+"], [0,0,0], [0,1,0])\n"
      "WriteImage('Uy_above.jpg')\n"
    )
  );
  results->insert("UyContour",
    std::auto_ptr<Image>(new Image
    (
    "Uy_above.jpg", 
    "Contour of Y-Velocity", ""
  )));

  runPvPython
  (
    runCase, dir, list_of<std::string>
    (
      init+
      "displayContour(eb, 'U', component=2)\n"
      "setCam([0,0,"+lexical_cast<std::string>(Lz)+"], [0,0,0], [0,1,0])\n"
      "WriteImage('Uz_above.jpg')\n"
    )
  );
  results->insert("UzContour",
    std::auto_ptr<Image>(new Image
    (
    "Uz_above.jpg", 
    "Contour of Z-Velocity", ""
  )));
  
  
  /*
  ptr_map_insert<ScalarResult>(*results) ("Excentricity", ex, "Excentricity of the shaft", "", "");
  ptr_map_insert<ScalarResult>(*results) ("PsiEff", PsiEff, "Realtive bearing clearance", "", "");
  ptr_map_insert<ScalarResult>(*results) ("Re", Re, "Reynolds number", "", "");
  ptr_map_insert<ScalarResult>(*results) ("ReLim", ReLim, "Laminar limit Reynolds number", "", "");
  ptr_map_insert<ScalarResult>(*results) ("PivotTorque", rho*m(2)*fac, "Torque on bearing shaft", "", "Nm");
  ptr_map_insert<ScalarResult>(*results) ("Power", rho*m(2)*fac * 2.*M_PI*rpm/60., "Power required to drive shaft", "", "W");
  ptr_map_insert<ScalarResult>(*results) ("VerticalPivotForce", rho*f(1)*fac, "Force on bearing shaft in vertical direction", "", "N");
  ptr_map_insert<ScalarResult>(*results) ("HorizontalPivotForce", rho*f(0)*fac, "Force on bearing shaft in horizontal direction", "", "N");
  ptr_map_insert<ScalarResult>(*results) ("ResultantPivotForce", rho*sqrt( f(0)*f(0) + f(1)*f(1) ) *fac, "Resultant force on bearing shaft", "", "N");
  ptr_map_insert<ScalarResult>(*results) ("PivotForceAngle", 180.* atan2(f(0), f(1)) /M_PI, "Angular direction of force on bearing shaft", "", "deg");
  ptr_map_insert<ScalarResult>(*results) ("CavitationMargin", pambient+(9.81*depthByD*Di+minp)*rho-pcav, 
					  "Difference between minimum pressure and cavitation pressure", "", "Pa");
*/
  return results;
}


addToFactoryTable(Analysis, ExplicitVortex, NoParameters);

}
