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
  
defineType(Pipe);

Pipe::Pipe(const NoParameters&)
: OpenFOAMAnalysis
  (
    "Pipe Flow Testcase",
    "Cylindrical domain with cyclic BCs on axial ends"
  )
{}

Pipe::~Pipe()
{

}

ParameterSet Pipe::defaultParameters() const
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
	    ("D",		new DoubleParameter(1.0, "[m] Diameter of the pipe"))
	    ("L",		new DoubleParameter(1.0, "[m] Length of the pipe"))
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
	    ("nc",	new IntParameter(20, "# cells in circumferential direction"))
	    ("nr",	new IntParameter(20, "# cells in radial direction"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Properties of the computational mesh"
	))
      
      ("operation", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("Re_tau",		new DoubleParameter(100, "[-] Friction-Velocity-Reynolds number"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Definition of the operation point under consideration"
	))
      
      ("fluid", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
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

void Pipe::cancel()
{
  stopFlag_=true;
}

void Pipe::createMesh
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
  PSINT(p, "mesh", nc);
  PSINT(p, "mesh", nr);
    
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "walls");
  
  std::map<int, Point> pts;
  
  double al = M_PI/2.;
  double Lc=0.33*L;
  
  pts = boost::assign::map_list_of 
    
      (10, 	vec3(0, 0.5*D, 0))
      /*
      (1, 	vec3(0, 0.5*D*cos(1.5*al), 0.5*D*sin(1.5*al)))
      (2, 	vec3(0, 0.5*D*cos(2.5*al), 0.5*D*sin(2.5*al)))
      (3, 	vec3(0, 0.5*D*cos(3.5*al), 0.5*D*sin(3.5*al)))
      */
      (11, 	vec3(0,  sqrt(2.)*Lc, 0.))
      /*
      (5, 	vec3(0, -0.5*Lc,  0.5*Lc))
      (6, 	vec3(0, -0.5*Lc, -0.5*Lc))
      (7, 	vec3(0,  0.5*Lc, -0.5*Lc))
      */
  ;
  arma::mat vL=vec3(-L, 0, 0);
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
	  (r1*pts[10])+L, (r2*pts[10])+L, (r3*pts[10])+L, (r0*pts[10])+L
	),
	nc, nc, nax
      )
    );
    cycl_in.addFace(bl.face("0321"));
    cycl_out.addFace(bl.face("4567"));
  }
  /*
  for (int i=0; i<4; i++)
  {
    arma::mat r0=rotMatrix(double(i+0.5)*al, ax);
    arma::mat r1=rotMatrix(double(i+1.5)*al, ax);
    
    Block& bl = bmd->addBlock
    (
      new Block(P_8(
	  r1*pts[10], r0*pts[10], r0*pts[11], r1*pts[11],
	  (r1*pts[10])+L, (r0*pts[10])+L, (r0*pts[11])+L, (r1*pts[11])+L
	),
	nc, nr, nax
      )
    );
    cycl_in.addFace(bl.face("0321"));
    cycl_out.addFace(bl.face("4567"));
  }
  */
  cm.insert(bmd.release());

  cm.createOnDisk(dir);
  cm.executeCommand(dir, "blockMesh");  
}

void Pipe::createCase
(
  OpenFOAMCase& cm,
  const ParameterSet& p
)
{
  // create local variables from ParameterSet
  PSDBL(p, "operation", Re_tau);
  PSINT(p, "fluid", RASModel);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);
    
  cm.insert(new simpleFoamNumerics(cm));
  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(1./Re_tau) ));
  
  //cm.insert(new VelocityInletBC(cm, "sides", boundaryDict, VelocityInletBC::Parameters() ));
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());
  
  insertTurbulenceModel(cm, p.get<SelectionParameter>("fluid/RASModel").selection());
  
  //cm.createOnDisk(dir);
  boost::shared_ptr<OFdicts> dicts=cm.createDictionaries();
  //dicts->lookupDict("system/fvSolution").subDict("solvers").subDict("U") = stdAsymmSolverSetup(1e-7, 0.01);  
  //dicts->lookupDict("system/fvSolution").subDict("solvers").subDict("p") = stdSymmSolverSetup(1e-7, 0.01);  
  cm.createOnDisk(dir, dicts);
/*  
  cm.executeCommand(dir, "setVortexVelocity",
    list_of<std::string>
    (lexical_cast<std::string>(Gamma))
    ("("+lexical_cast<std::string>(vcx)+" "+lexical_cast<std::string>(vcy)+" 0)")
  );  
  */
}

ResultSetPtr Pipe::operator()(ProgressDisplayer* displayer)
{
  const ParameterSet& p = *parameters_;
  
  PSDBL(p, "geometry", D);
  PSDBL(p, "geometry", L);

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
  /*
  SolverOutputAnalyzer analyzer(*displayer);
  runCase.runSolver(dir, analyzer, "simpleFoam", &stopFlag_);
*/
  ResultSetPtr results(new ResultSet(p, "Circular Journal Bearing Analysis", "Result Report"));
  /*
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
  */
  
  return results;
}


addToFactoryTable(Analysis, Pipe, NoParameters);


}
