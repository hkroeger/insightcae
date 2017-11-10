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

#include "decayingturbulence.h"
#include "base/factory.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/openfoamcaseelements.h"

#include <boost/assign/list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"

using namespace arma;
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;

namespace insight
{
  
addToAnalysisFactoryTable(DecayingTurbulence);

DecayingTurbulence::DecayingTurbulence(const ParameterSet& ps, const boost::filesystem::path& exepath)
: OpenFOAMAnalysis
  (
    "Decaying Turbulence Test Case",
    "Turbulent flow entering domain and decaying inside",
    ps, exepath
  ),
  inlet_("inlet"),
  outlet_("outlet")
{}

DecayingTurbulence::~DecayingTurbulence()
{}

ParameterSet DecayingTurbulence::defaultParameters()
{
  ParameterSet p(Parameters::makeDefault());
  p.merge(OpenFOAMAnalysis::defaultParameters());
  return p;
}

double DecayingTurbulence::calcT() const
{
  Parameters p(parameters_);
  return p.geometry.L/p.operation.U;
}


void DecayingTurbulence::createCase(insight::OpenFOAMCase& cm)
{
  Parameters p(parameters_);
 // create local variables from ParameterSet
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

  double T=calcT();
  cout << "Flow-through time T="<<T<<endl;
  cm.insert(new pimpleFoamNumerics(cm) );
  cm.insert(new fieldAveraging(cm, fieldAveraging::Parameters()
    .set_fields(list_of<std::string>("p")("U"))
    .set_name("averaging")
    .set_timeStart(T)
  ));
  
  /*
  cm.insert(new probes(cm, probes::Parameters()
    .set_name("probes")
    .set_fields(list_of<std::string>("p")("U"))
    .set_timeStart(T)
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
  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters().set_nu(p.fluid.nu) ));
  
  cm.insert(new TurbulentVelocityInletBC(cm, inlet_, boundaryDict/*, TurbulentVelocityInletBC::Parameters()
    .set_velocity(FieldData(vec3(U, 0, 0)))
    .set_turbulence(uniformIntensityAndLengthScale(0.05,0.1*H))*/
  ));
  
  cm.insert(new PressureOutletBC(cm, outlet_, boundaryDict, PressureOutletBC::Parameters()
    .set_pressure(0.0) 
  ));
  
  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());
  
  insertTurbulenceModel(cm, parameters_.get<SelectionParameter>("fluid/turbulenceModel").selection());
}


int DecayingTurbulence::calcnh() const
{
  Parameters p(parameters_);
  double Delta=p.geometry.L/double(p.mesh.nax);
  return p.geometry.H/(Delta/p.mesh.s);
}

void DecayingTurbulence::createMesh(insight::OpenFOAMCase& cm)
{
  // create local variables from ParameterSet
  path dir = executionPath();
  Parameters p(parameters_);
    
  int nh=calcnh();
    
  cm.insert(new MeshingNumerics(cm));
  
  using namespace insight::bmd;
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "wall");
  
  
  double al = M_PI/2.;
  
  std::map<int, Point> pts;
  pts = boost::assign::map_list_of   
      (0, 	vec3(0, -0.5*p.geometry.H, -0.5*p.geometry.H))
      (1, 	vec3(0, -0.5*p.geometry.H, 0.5*p.geometry.H))
      (2, 	vec3(0, 0.5*p.geometry.H, 0.5*p.geometry.H))
      (3, 	vec3(0, 0.5*p.geometry.H, -0.5*p.geometry.H))
      .convert_to_container<std::map<int, Point> >()
  ;
  arma::mat vL=vec3(p.geometry.L, 0, 0);
  arma::mat ax=vec3(1, 0, 0);
  
  // create patches
  Patch& in= 	bmd->addPatch(inlet_, new Patch());
  Patch& out= 	bmd->addPatch(outlet_, new Patch());
  Patch& upDown= bmd->addPatch("upDown", new Patch("cyclic"));
  Patch& frontBack= bmd->addPatch("frontBack", new Patch("cyclic"));
  
  {
    Block& bl = bmd->addBlock
    (  
      new Block(P_8(
	  pts[0]+vL, pts[1]+vL, pts[2]+vL, pts[3]+vL,
	  pts[0], pts[1], pts[2], pts[3]
	),
	nh, nh, p.mesh.nax
      )
    );
    in.addFace(bl.face("0321"));
    out.addFace(bl.face("4567"));
    upDown.addFace(bl.face("0154"));
    upDown.addFace(bl.face("2376"));
    frontBack.addFace(bl.face("1265"));
    frontBack.addFace(bl.face("0473"));
  }

  cm.insert(bmd.release());

  cm.createOnDisk(dir);
  cm.executeCommand(dir, "blockMesh");  
}

void DecayingTurbulence::applyCustomPreprocessing(OpenFOAMCase& cm)
{
  /*
  setFields(cm, executionPath(), 
	    list_of<setFieldOps::FieldValueSpec>
	      ("volVectorFieldValue U ("+lexical_cast<string>(U)+" 0 0)"),
	    ptr_vector<setFieldOps::setFieldOperator>()
  );
  
  cm.get<TurbulentVelocityInletBC>(inlet_+"BC")->initInflowBC(executionPath());
  */
  OpenFOAMAnalysis::applyCustomPreprocessing(cm);
}

void DecayingTurbulence::applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts)
{
  OpenFOAMAnalysis::applyCustomOptions(cm, dicts);
  
  OFDictData::dictFile& controlDict=dicts->addDictionaryIfNonexistent("system/controlDict");
  /*
  if (cm.OFversion()<=160)
  {
    controlDict["application"]="channelFoam";
  }
  */
  controlDict["endTime"]=10.0*calcT();
}

}
