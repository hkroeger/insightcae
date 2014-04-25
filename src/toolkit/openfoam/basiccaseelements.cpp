/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  hannes <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "basiccaseelements.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"

#include <utility>
#include "boost/assign.hpp"
#include "boost/lexical_cast.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;

namespace insight
{


thermodynamicModel::thermodynamicModel(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "thermodynamicModel")
{
}

cavitatingFoamThermodynamics::cavitatingFoamThermodynamics(OpenFOAMCase& c, const Parameters& p)
: thermodynamicModel(c),
  p_(p)
{
}

void cavitatingFoamThermodynamics::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& thermodynamicProperties=dictionaries.addDictionaryIfNonexistent("constant/thermodynamicProperties");
  thermodynamicProperties["barotropicCompressibilityModel"]="linear";
  thermodynamicProperties["psiv"]=OFDictData::dimensionedData("psiv", 
							      OFDictData::dimension(0, -2, 2), 
							      p_.psiv());
  thermodynamicProperties["rholSat"]=OFDictData::dimensionedData("rholSat", 
								 OFDictData::dimension(1, -3), 
								 p_.rholSat());
  thermodynamicProperties["psil"]=OFDictData::dimensionedData("psil", 
								 OFDictData::dimension(0, -2, 2), 
								 p_.psil());
  thermodynamicProperties["pSat"]=OFDictData::dimensionedData("pSat", 
								 OFDictData::dimension(1, -1, -2), 
								 p_.pSat());
  thermodynamicProperties["rhoMin"]=OFDictData::dimensionedData("rhoMin", 
								 OFDictData::dimension(1, -3), 
								 p_.rhoMin());
}

gravity::gravity(OpenFOAMCase& c, Parameters const& p)
: OpenFOAMCaseElement(c, "gravity"),
  p_(p)
{
}

void gravity::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& g=dictionaries.addDictionaryIfNonexistent("constant/g");
  g["dimensions"]="[0 1 -2 0 0 0 0]";
  OFDictData::list gv;
  for (int i=0; i<3; i++) gv.push_back(p_.g()(i));
  g["value"]=gv;
}
  
transportModel::transportModel(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "transportModel")
{
}

MRFZone::MRFZone(OpenFOAMCase& c, Parameters const& p )
: OpenFOAMCaseElement(c, "MRFZone"+p.name()),
  p_(p)
{
}

void MRFZone::addIntoDictionaries(OFdicts& dictionaries) const
{
  if (OFversion()<220)
  {
    throw insight::Exception("Not yet supported!");
  }
  else
  {
    OFDictData::dict coeffs;
    OFDictData::list nrp; nrp.resize(p_.nonRotatingPatches().size());
    copy(p_.nonRotatingPatches().begin(), p_.nonRotatingPatches().end(), nrp.begin());
    
    coeffs["nonRotatingPatches"]=nrp;
    coeffs["origin"]=OFDictData::vector3(p_.rotationCentre());
    coeffs["axis"]=OFDictData::vector3(p_.rotationAxis());
    coeffs["omega"]=2.*M_PI*p_.rpm()/60.;

    OFDictData::dict fod;
    fod["type"]="MRFSource";
    fod["active"]=true;
    fod["selectionMode"]="cellZone";
    fod["cellZone"]=p_.name();
    fod["MRFSourceCoeffs"]=coeffs;
    
    OFDictData::dict& fvOptions=dictionaries.addDictionaryIfNonexistent("system/fvOptions");
    fvOptions[p_.name()]=fod;     
  }
}

PressureGradientSource::PressureGradientSource(OpenFOAMCase& c, Parameters const& p )
: OpenFOAMCaseElement(c, "PressureGradientSource"),
  p_(p)
{
}

void PressureGradientSource::addIntoDictionaries(OFdicts& dictionaries) const
{
  if (OFversion()>=220)
  {
    OFDictData::dict coeffs;    
    OFDictData::list flds; flds.push_back("U");
    coeffs["fieldNames"]=flds;
    coeffs["Ubar"]=OFDictData::vector3(p_.Ubar());

    OFDictData::dict fod;
    fod["type"]="pressureGradientExplicitSource";
    fod["active"]=true;
    fod["selectionMode"]="all";
    fod["pressureGradientExplicitSourceCoeffs"]=coeffs;
    
    OFDictData::dict& fvOptions=dictionaries.addDictionaryIfNonexistent("system/fvOptions");
    fvOptions[name()]=fod;  
  }
  else
  {
    // for channelFoam:
    OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
    transportProperties["Ubar"]=OFDictData::dimensionedData("Ubar", dimVelocity, OFDictData::vector3(p_.Ubar()));
  }
}

singlePhaseTransportProperties::singlePhaseTransportProperties(OpenFOAMCase& c, Parameters const& p )
: transportModel(c),
  p_(p)
{
}
 
void singlePhaseTransportProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
  transportProperties["transportModel"]="Newtonian";
  transportProperties["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimension(0, 2, -1), p_.nu());
}

twoPhaseTransportProperties::twoPhaseTransportProperties(OpenFOAMCase& c, Parameters const& p )
: transportModel(c),
  p_(p)
{
}
 
void twoPhaseTransportProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
  
  OFDictData::dict& twoPhase=transportProperties.addSubDictIfNonexistent("twoPhase");
  twoPhase["transportModel"]="twoPhase";
  twoPhase["phase1"]="phase1";
  twoPhase["phase2"]="phase2";
  
  OFDictData::dict& phase1=transportProperties.addSubDictIfNonexistent("phase1");
  phase1["transportModel"]="Newtonian";
  phase1["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimension(0, 2, -1), p_.nu1());
  phase1["rho"]=OFDictData::dimensionedData("rho", OFDictData::dimension(1, -3), p_.rho1());
  
  OFDictData::dict& phase2=transportProperties.addSubDictIfNonexistent("phase2");
  phase2["transportModel"]="Newtonian";
  phase2["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimension(0, 2, -1), p_.nu2());
  phase2["rho"]=OFDictData::dimensionedData("rho", OFDictData::dimension(1, -3), p_.rho2());

  transportProperties["sigma"]=OFDictData::dimensionedData("sigma", OFDictData::dimension(1, 0, -2), p_.sigma());

}

namespace phaseChangeModels
{
  
phaseChangeModel::~phaseChangeModel()
{
}

SchnerrSauer::SchnerrSauer(Parameters const& p)
: p_(p)
{
}

void SchnerrSauer::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
  transportProperties["phaseChangeTwoPhaseMixture"]="SchnerrSauer";
  
  OFDictData::dict& coeffs=transportProperties.addSubDictIfNonexistent("SchnerrSauerCoeffs");
  coeffs["n"] = OFDictData::dimensionedData("n", OFDictData::dimension(0, -3), p_.n());
  coeffs["dNuc"] = OFDictData::dimensionedData("dNuc", OFDictData::dimension(0, 1), p_.dNuc());
  coeffs["Cc"] = OFDictData::dimensionedData("Cc", OFDictData::dimension(), p_.Cc());
  coeffs["Cv"] = OFDictData::dimensionedData("Cv", OFDictData::dimension(), p_.Cv());
}

}

cavitationTwoPhaseTransportProperties::cavitationTwoPhaseTransportProperties
(
  OpenFOAMCase& c, 
  Parameters const& p
)
: twoPhaseTransportProperties(c, p),
  p_(p)
{
}

void cavitationTwoPhaseTransportProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  twoPhaseTransportProperties::addIntoDictionaries(dictionaries);
  OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
  transportProperties["pSat"]=OFDictData::dimensionedData("pSat", OFDictData::dimension(1, -1, -2), p_.psat());
  p_.model()->addIntoDictionaries(dictionaries);
}
  
  
dynamicMesh::dynamicMesh(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "dynamicMesh")
{
}

velocityTetFEMMotionSolver::velocityTetFEMMotionSolver(OpenFOAMCase& c)
: dynamicMesh(c),
  tetFemNumerics_(c)
{
  c.addField("motionU", FieldInfo(vectorField, 	dimVelocity, 		list_of(0.0)(0.0)(0.0), tetField ) );
}

void velocityTetFEMMotionSolver::addIntoDictionaries(OFdicts& dictionaries) const
{
  tetFemNumerics_.addIntoDictionaries(dictionaries);

  OFDictData::dict& tetFemSolution=dictionaries.addDictionaryIfNonexistent("system/tetFemSolution");
  OFDictData::dict& solvers = tetFemSolution.subDict("solvers");
  solvers["motionU"]=stdSymmSolverSetup();
  
  OFDictData::dict& dynamicMeshDict=dictionaries.addDictionaryIfNonexistent("constant/dynamicMeshDict");
  dynamicMeshDict["dynamicFvMesh"]=OFDictData::data("dynamicMotionSolverFvMesh");
  dynamicMeshDict["solver"]=OFDictData::data("laplaceFaceDecomposition");
  if (dynamicMesh::OFversion()<=160)
  {
    dynamicMeshDict["diffusivity"]=OFDictData::data("uniform");
    dynamicMeshDict["frozenDiffusion"]=OFDictData::data(false);
    dynamicMeshDict["twoDMotion"]=OFDictData::data(false);
  }
  else
  {
    throw insight::Exception("No tetFEMMotionsolver available for OF>1.6 ext");
  }
}

displacementFvMotionSolver::displacementFvMotionSolver(OpenFOAMCase& c)
: dynamicMesh(c)
{
}

void displacementFvMotionSolver::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& dynamicMeshDict=dictionaries.addDictionaryIfNonexistent("constant/dynamicMeshDict");
  dynamicMeshDict["dynamicFvMesh"]=OFDictData::data("dynamicMotionSolverFvMesh");
  dynamicMeshDict["solver"]=OFDictData::data("displacementLaplacian");
  if (OFversion()<220)
  {
    dynamicMeshDict["diffusivity"]=OFDictData::data("uniform");
  }
  else
  {
    OFDictData::dict sd;
    sd["diffusivity"]=OFDictData::data("uniform");
    dynamicMeshDict["displacementLaplacianCoeffs"]=sd;
  }
}


defineType(turbulenceModel);
defineFactoryTable(turbulenceModel, turbulenceModel::ConstrP);

turbulenceModel::turbulenceModel(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "turbulenceModel")
{
}

turbulenceModel::turbulenceModel(const turbulenceModel::ConstrP& c)
: OpenFOAMCaseElement(c.get<0>(), "turbulenceModel")
{
}


defineType(RASModel);

RASModel::RASModel(OpenFOAMCase& c)
: turbulenceModel(c)
{
}

RASModel::RASModel(const turbulenceModel::ConstrP& c)
: turbulenceModel(c)
{
}

void RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& turbProperties=dictionaries.addDictionaryIfNonexistent("constant/turbulenceProperties");
  turbProperties["simulationType"]="RASModel";
}

turbulenceModel::AccuracyRequirement RASModel::minAccuracyRequirement() const
{
  return AC_RANS;
}

defineType(LESModel);

LESModel::LESModel(OpenFOAMCase& c)
: turbulenceModel(c)
{
}

LESModel::LESModel(const turbulenceModel::ConstrP& c)
: turbulenceModel(c)
{
}

void LESModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& turbProperties=dictionaries.addDictionaryIfNonexistent("constant/turbulenceProperties");
  turbProperties["simulationType"]="LESModel";
}

turbulenceModel::AccuracyRequirement LESModel::minAccuracyRequirement() const
{
  return AC_LES;
}

defineType(laminar_RASModel);
addToFactoryTable(turbulenceModel, laminar_RASModel, turbulenceModel::ConstrP);

laminar_RASModel::laminar_RASModel(OpenFOAMCase& c)
: RASModel(c)
{}
  
laminar_RASModel::laminar_RASModel(const turbulenceModel::ConstrP& c)
: RASModel(c)
{}
  
void laminar_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);
  
  OFDictData::dict& RASProperties=dictionaries.addDictionaryIfNonexistent("constant/RASProperties");
  RASProperties["RASModel"]="laminar";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.addSubDictIfNonexistent("laminarCoeffs");
}

bool laminar_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  return false;
}

defineType(oneEqEddy_LESModel);
addToFactoryTable(turbulenceModel, oneEqEddy_LESModel, turbulenceModel::ConstrP);

void oneEqEddy_LESModel::addFields()
{
  OFcase().addField("k", 	FieldInfo(scalarField, 	dimKinEnergy, 	list_of(1e-10), volField ) );
  OFcase().addField("nuSgs", 	FieldInfo(scalarField, 	dimKinViscosity, 	list_of(1e-10), volField ) );
}
  

oneEqEddy_LESModel::oneEqEddy_LESModel(OpenFOAMCase& c)
: LESModel(c)
{
  addFields();
}

oneEqEddy_LESModel::oneEqEddy_LESModel(const ConstrP& c)
: LESModel(c)
{
  addFields();
}

void oneEqEddy_LESModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  LESModel::addIntoDictionaries(dictionaries);
  
  OFDictData::dict& LESProperties=dictionaries.addDictionaryIfNonexistent("constant/LESProperties");
  LESProperties["LESModel"]="oneEqEddy";
  LESProperties["delta"]="cubeRootVol";
  LESProperties["printCoeffs"]=true;
  OFDictData::dict crvc;
  crvc["deltaCoeff"]=1.0;
  LESProperties["cubeRootVolCoeffs"]=crvc;
  LESProperties.addSubDictIfNonexistent("laminarCoeffs");
}

bool oneEqEddy_LESModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (fieldname == "k")
  {
    BC["type"]="fixedValue";
    BC["value"]="uniform 0";
    return true;
  }
  else if (fieldname == "nuSgs")
  {
    BC["type"]="zeroGradient";
    return true;
  }
  
  return false;
}

defineType(dynSmagorinsky_LESModel);
addToFactoryTable(turbulenceModel, dynSmagorinsky_LESModel, turbulenceModel::ConstrP);

void dynSmagorinsky_LESModel::addFields()
{
  OFcase().addField("k", 	FieldInfo(scalarField, 	dimKinEnergy, 	list_of(1e-10), volField ) );
  OFcase().addField("nuSgs", 	FieldInfo(scalarField, 	dimKinViscosity, 	list_of(1e-10), volField ) );
}

dynSmagorinsky_LESModel::dynSmagorinsky_LESModel(OpenFOAMCase& c)
: LESModel(c)
{
  addFields();
}

dynSmagorinsky_LESModel::dynSmagorinsky_LESModel(const ConstrP& c)
: LESModel(c)
{
  addFields();
}


void dynSmagorinsky_LESModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  LESModel::addIntoDictionaries(dictionaries);
  
  OFDictData::dict& LESProperties=dictionaries.addDictionaryIfNonexistent("constant/LESProperties");
  
  string modelName="dynSmagorinsky";
  if (OFversion()>160)
    modelName="homogeneousDynSmagorinsky";
  
  LESProperties["LESModel"]=modelName;
  LESProperties["delta"]="cubeRootVol";
  LESProperties["printCoeffs"]=true;
  
  OFDictData::dict crvc;
  crvc["deltaCoeff"]=1.0;
  LESProperties["cubeRootVolCoeffs"]=crvc;
  OFDictData::dict& cd=LESProperties.addSubDictIfNonexistent(modelName+"Coeffs");
  cd["filter"]="simple";
}

bool dynSmagorinsky_LESModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (fieldname == "k")
  {
    BC["type"]="fixedValue";
    BC["value"]="uniform 0";
    return true;
  }
  else if (fieldname == "nuSgs")
  {
    BC["type"]="zeroGradient";
    return true;
  }
  
  return false;
}


defineType(kOmegaSST_RASModel);
addToFactoryTable(turbulenceModel, kOmegaSST_RASModel, turbulenceModel::ConstrP);

void kOmegaSST_RASModel::addFields()
{
  OFcase().addField("k", 	FieldInfo(scalarField, 	dimKinEnergy, 	list_of(1e-10), volField ) );
  OFcase().addField("omega", 	FieldInfo(scalarField, 	OFDictData::dimension(0, 0, -1), 	list_of(1.0), volField ) );
  OFcase().addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	list_of(1e-10), volField ) );
}

kOmegaSST_RASModel::kOmegaSST_RASModel(OpenFOAMCase& c)
: RASModel(c)
{
  addFields();
}
  
kOmegaSST_RASModel::kOmegaSST_RASModel(const turbulenceModel::ConstrP& c)
: RASModel(c)
{
  addFields();
}
  
void kOmegaSST_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=dictionaries.addDictionaryIfNonexistent("constant/RASProperties");
  RASProperties["RASModel"]="kOmegaSST";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.addSubDictIfNonexistent("kOmegaSSTCoeffs");
}

bool kOmegaSST_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (fieldname == "k")
  {
    BC["type"]=OFDictData::data("kqRWallFunction");
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  else if (fieldname == "omega")
  {
    BC["type"]=OFDictData::data("omegaWallFunction");
    BC["Cmu"]=0.09;
    BC["kappa"]=0.41;
    BC["E"]=9.8;
    BC["beta1"]=0.075;
    BC["value"]="uniform 1";
    return true;
  }
  else if (fieldname == "nut")
  {
    BC["type"]=OFDictData::data("nutkWallFunction");
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  
  return false;
}

defineType(LEMOSHybrid_RASModel);
addToFactoryTable(turbulenceModel, LEMOSHybrid_RASModel, turbulenceModel::ConstrP);

void LEMOSHybrid_RASModel::addFields()
{
  OFcase().addField("kSgs", 	FieldInfo(scalarField, 	dimKinEnergy, 	list_of(1e-10), volField ) );
  OFcase().addField("nuSgs", 	FieldInfo(scalarField, 	dimKinViscosity, 	list_of(1e-10), volField ) );
  OFcase().addField("UAvgHyb", 	FieldInfo(vectorField, 	dimVelocity, 	list_of(0)(0)(0), volField ) );
}

LEMOSHybrid_RASModel::LEMOSHybrid_RASModel(OpenFOAMCase& c)
: kOmegaSST_RASModel(c)
{
  addFields();
}

LEMOSHybrid_RASModel::LEMOSHybrid_RASModel(const ConstrP& c)
: kOmegaSST_RASModel(c)
{
  addFields();
}


void LEMOSHybrid_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  // add k-O stuff first, we will overwrite afterwards, where necessary
  kOmegaSST_RASModel::addIntoDictionaries(dictionaries);
  
  OFDictData::dict& RASProperties=dictionaries.addDictionaryIfNonexistent("constant/RASProperties");

  string modelName="hybKOmegaSST2";

  if (OFversion()<230)
    throw insight::Exception("The LES model "+modelName+" is unsupported in the selected OF version!");
    
  RASProperties["RASModel"]=modelName;
  RASProperties["delta"]="maxEdge";
  RASProperties["printCoeffs"]=true;
  
  OFDictData::dict mec;
  mec["deltaCoeff"]=1.0;
  RASProperties["maxEdgeCoeffs"]=mec;
  
  OFDictData::dict& cd=RASProperties.addSubDictIfNonexistent(modelName+"Coeffs");
  cd["filter"]="simple";
  cd["x1"]=1.0;
  cd["x2"]=2.0;
  cd["Cint"]=1.0;
  cd["CN"]=1.0;

  cd["averagingTime"]=1;
  cd["fixedInterface"]=false;
  cd["useIDDESDelta"]=false;

  cd["delta"]="maxEdge";

  cd["cubeRootVolCoeffs"]=mec;
  cd["IDDESDeltaCoeffs"]=mec;
  cd["maxEdgeCoeffs"]=mec;

  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict.getList("libs").push_back( OFDictData::data("\"libLEMOS-2.3.x.so\"") );  
}

bool LEMOSHybrid_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (!kOmegaSST_RASModel::addIntoFieldDictionary(fieldname, fieldinfo, BC))
  {
    if (fieldname == "kSgs")
    {
      BC["type"]="fixedValue";
      BC["value"]="uniform 0";
      return true;
    }
    else if (fieldname == "nuSgs")
    {
      BC["type"]="zeroGradient";
      return true;
    }
  }
  
  return false;
}

defineType(kOmegaSST_LowRe_RASModel);
addToFactoryTable(turbulenceModel, kOmegaSST_LowRe_RASModel, turbulenceModel::ConstrP);

kOmegaSST_LowRe_RASModel::kOmegaSST_LowRe_RASModel(OpenFOAMCase& c)
: kOmegaSST_RASModel(c)
{}
  
kOmegaSST_LowRe_RASModel::kOmegaSST_LowRe_RASModel(const turbulenceModel::ConstrP& c)
: kOmegaSST_RASModel(c)
{}
  
void kOmegaSST_LowRe_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=dictionaries.addDictionaryIfNonexistent("constant/RASProperties");
  RASProperties["RASModel"]="kOmegaSST_LowRe";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.addSubDictIfNonexistent("kOmegaSST_LowReCoeffs");
}

bool kOmegaSST_LowRe_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if ( (fieldname == "k") || (fieldname == "omega") )
  {
    BC["type"]=OFDictData::data("zeroGradient");
    return true;
  }
  return false;
}

defineType(kOmegaSST2_RASModel);
addToFactoryTable(turbulenceModel, kOmegaSST2_RASModel, turbulenceModel::ConstrP);

void kOmegaSST2_RASModel::addFields()
{
  OFcase().addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	list_of(1e-10), volField ) );
}

kOmegaSST2_RASModel::kOmegaSST2_RASModel(OpenFOAMCase& c)
: kOmegaSST_RASModel(c)
{
  kOmegaSST2_RASModel::addFields();
}
  
kOmegaSST2_RASModel::kOmegaSST2_RASModel(const turbulenceModel::ConstrP& c)
: kOmegaSST_RASModel(c)
{
  kOmegaSST2_RASModel::addFields();  
}
  
void kOmegaSST2_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=dictionaries.addDictionaryIfNonexistent("constant/RASProperties");
  RASProperties["RASModel"]="kOmegaSST2";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.addSubDictIfNonexistent("kOmegaSST2");
  
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict.getList("libs").push_back( OFDictData::data("\"libkOmegaSST2.so\"") );
}

bool kOmegaSST2_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (fieldname == "k")
  {
    BC["type"]="kqRWallFunction";
    BC["value"]="uniform 1e-10";
    return true;
  }
  else if (fieldname == "omega")
  {
    BC["type"]="hybridOmegaWallFunction2";
    BC["Cmu"]=0.09;
    BC["kappa"]=0.41;
    BC["E"]=9.8;
    BC["tw"]=0.057;
    BC["value"]="uniform 1";
    return true;
  }
  else if (fieldname == "nut")
  {
    BC["type"]="nutHybridWallFunction2";
    BC["Cmu"]=0.09;
    BC["kappa"]=0.41;
    BC["E"]=9.8;
    BC["value"]="uniform 1e-10";
    return true;
  }
  return false;
}


BoundaryCondition::BoundaryCondition(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict)
: OpenFOAMCaseElement(c, patchName+"BC"),
  patchName_(patchName),
  type_(boundaryDict.subDict(patchName).getString("type")),
  nFaces_(boundaryDict.subDict(patchName).getInt("nFaces")),
  startFace_(boundaryDict.subDict(patchName).getInt("startFace"))  
{
}

void BoundaryCondition::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["type"]=type_;
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
}

void BoundaryCondition::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dictFile& fieldDict=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second);
    OFDictData::dict& boundaryField=fieldDict.addSubDictIfNonexistent("boundaryField");
    OFDictData::dict& BC=boundaryField.addSubDictIfNonexistent(patchName_);
  }
}

void BoundaryCondition::addIntoDictionaries(OFdicts& dictionaries) const
{
  addIntoFieldDictionaries(dictionaries);
  OFDictData::dict bndsubd;
  addOptionsToBoundaryDict(bndsubd);
  
  insertIntoBoundaryDict(dictionaries, patchName_, bndsubd);
}

void BoundaryCondition::insertIntoBoundaryDict
(
  OFdicts& dictionaries, 
  const string& patchName,
  const OFDictData::dict& bndsubd
)
{
  // contents is created as list of string / subdict pairs
  // patches have to appear ordered by "startFace"!
  OFDictData::dict& boundaryDict=dictionaries.addDictionaryIfNonexistent("constant/polyMesh/boundary");
  if (boundaryDict.size()==0)
    boundaryDict.addListIfNonexistent("");
  
  OFDictData::list& bl=
    *boost::get<OFDictData::list>( &boundaryDict.begin()->second );
  
  //std::cout<<"Configuring "<<patchName_<<std::endl;
  // search, if patchname is already present; replace, if yes
  for(OFDictData::list::iterator i=bl.begin(); i!=bl.end(); i++)
  {
    if (std::string *name = boost::get<std::string>(&(*i)))
    {
      //cout<<"found "<<*name<<endl;
      if ( *name == patchName )
      {
	i++;
	*i=bndsubd;
	return;
      }
    }
  }
  
  // not found, insert (at the right location)
  OFDictData::list::iterator j = bl.end();
  for(OFDictData::list::iterator i=bl.begin(); i!=bl.end(); i++)
  {
    if (OFDictData::dict *d = boost::get<OFDictData::dict>(&(*i)))
    {
      if (d->getInt("startFace") > bndsubd.getInt("startFace") ) 
      {
	//std::cout << "Inserting before " << *boost::get<std::string>(&(*(i-1))) << std::endl;
	j=i-1;
	break;
      }
      // patch with 0 faces has to be inserted before the face with the same start address but nonzero size
      if ( (d->getInt("startFace") == bndsubd.getInt("startFace") ) && (bndsubd.getInt("nFaces") == 0) )
      {
	//std::cout << "Inserting before " << *boost::get<std::string>(&(*(i-1))) << std::endl;
	j=i-1;
	break;
      }
    }
  }
  j = bl.insert( j, OFDictData::data(patchName) );
  bl.insert( j+1, bndsubd );
  
  OFDictData::dict::iterator oe=boundaryDict.begin();
  std::swap( boundaryDict[lexical_cast<std::string>(bl.size()/2)], oe->second );
  boundaryDict.erase(oe);
}

bool BoundaryCondition::providesBCsForPatch(const std::string& patchName) const
{
  return (patchName == patchName_);
}


SimpleBC::SimpleBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const std::string className)
: BoundaryCondition(c, patchName, boundaryDict),
  className_(className)
{
  type_=className;
}

void SimpleBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    if ( (className_=="cyclic") && ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
      BC["type"]=OFDictData::data(className_);
  }
}


CyclicPairBC::CyclicPairBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict)
: OpenFOAMCaseElement(c, patchName+"CyclicBC"),
  patchName_(patchName)
{
  if (c.OFversion()>=210)
  {
    nFaces_=boundaryDict.subDict(patchName_+"_half0").getInt("nFaces");
    startFace_=boundaryDict.subDict(patchName_+"_half0").getInt("startFace");
    nFaces1_=boundaryDict.subDict(patchName_+"_half1").getInt("nFaces");
    startFace1_=boundaryDict.subDict(patchName_+"_half1").getInt("startFace");
  }
  else
  {
    nFaces_=boundaryDict.subDict(patchName_).getInt("nFaces");
    startFace_=boundaryDict.subDict(patchName_).getInt("startFace");
  }
}

void CyclicPairBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  addIntoFieldDictionaries(dictionaries);
  
  OFDictData::dict bndsubd, bndsubd1;
  bndsubd["type"]="cyclic";
  bndsubd["nFaces"]=nFaces_;
  bndsubd["startFace"]=startFace_;
  bndsubd1["type"]="cyclic";
  bndsubd1["nFaces"]=nFaces1_;
  bndsubd1["startFace"]=startFace1_;
  
  if (OFversion()>=210)
  {
    bndsubd["neighbourPatch"]=patchName_+"_half1";
    bndsubd1["neighbourPatch"]=patchName_+"_half0";
    BoundaryCondition::insertIntoBoundaryDict(dictionaries, patchName_+"_half0", bndsubd);
    BoundaryCondition::insertIntoBoundaryDict(dictionaries, patchName_+"_half1", bndsubd1);
  }
  else
  {
    BoundaryCondition::insertIntoBoundaryDict(dictionaries, patchName_, bndsubd);
  }
}

void CyclicPairBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dictFile& fieldDict=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second);
    OFDictData::dict& boundaryField=fieldDict.addSubDictIfNonexistent("boundaryField");
    
    if (OFversion()>=210)
    {
      OFDictData::dict& BC=boundaryField.addSubDictIfNonexistent(patchName_+"_half0");
      OFDictData::dict& BC1=boundaryField.addSubDictIfNonexistent(patchName_+"_half1");
      
      if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      {
	noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
	noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC1);
      }
      else
      {
	BC["type"]="cyclic";
	BC1["type"]="cyclic";
      }
    }
    else
    {
      OFDictData::dict& BC=boundaryField.addSubDictIfNonexistent(patchName_);
      
      if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      {
	noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
      }
      else
      {
	BC["type"]="cyclic";
      }
    }
  }
}

bool CyclicPairBC::providesBCsForPatch(const std::string& patchName) const
{
  if (OFversion()>=210)
    return ( (patchName == patchName_+"_half0") || (patchName == patchName_+"_half1") );
  else
    return ( patchName == patchName_ );
}

GGIBC::GGIBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, 
	Parameters const &p )
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
}

void GGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  if (OFversion()>=210)
  {
    bndDict["type"]="cyclicAMI";
    bndDict["neighbourPatch"]= p_.shadowPatch();
    bndDict["matchTolerance"]= 0.001;
    //bndDict["transform"]= "rotational";    
  }
  else
  {
    bndDict["type"]="ggi";
    bndDict["shadowPatch"]= p_.shadowPatch();
    bndDict["separationOffset"]=OFDictData::vector3(p_.separationOffset());
    bndDict["bridgeOverlap"]=p_.bridgeOverlap();
    bndDict["zone"]=p_.zone();
  }
}

void GGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    
    if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
    {
      if (OFversion()>=220)
	BC["type"]=OFDictData::data("cyclicAMI");
      else
	BC["type"]=OFDictData::data("ggi");
    }
  }
}


CyclicGGIBC::CyclicGGIBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, 
	Parameters const &p )
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
}

void CyclicGGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  if (OFversion()>=210)
  {
    bndDict["type"]="cyclicAMI";
    bndDict["neighbourPatch"]= p_.shadowPatch();
    bndDict["matchTolerance"]= 0.001;
    bndDict["transform"]= "rotational";    
    bndDict["rotationCentre"]=OFDictData::vector3(p_.rotationCentre());
    bndDict["rotationAxis"]=OFDictData::vector3(p_.rotationAxis());
    bndDict["rotationAngle"]=p_.rotationAngle();
  }
  else
  {
    bndDict["type"]="cyclicGgi";
    bndDict["shadowPatch"]= p_.shadowPatch();
    bndDict["separationOffset"]=OFDictData::vector3(p_.separationOffset());
    bndDict["bridgeOverlap"]=p_.bridgeOverlap();
    bndDict["rotationAxis"]=OFDictData::vector3(p_.rotationAxis());
    bndDict["rotationAngle"]=p_.rotationAngle();
    bndDict["zone"]=p_.zone();
  }
}

void CyclicGGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    
    if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
    {
      if (OFversion()>=220)
	BC["type"]=OFDictData::data("cyclicAMI");
      else
	BC["type"]=OFDictData::data("cyclicGgi");
    }
  }
}

namespace multiphaseBC
{
  
multiphaseBC::~multiphaseBC()
{
}


uniformPhases::uniformPhases( Parameters const& p )
: p_(p)
{}

bool uniformPhases::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  const PhaseFractionList& f = p_.phasefractions();
  if 
  (     
    (f.find(fieldname)!=f.end())
    && 
    (get<0>(fieldinfo)==scalarField) 
  )
  {
    BC["type"]="fixedValue";
    std::ostringstream entry;
    entry << "uniform "<<f.find(fieldname)->second;
    BC["value"]=entry.str();
    return true;
  }
  else 
    return false;
}

}

SuctionInletBC::SuctionInletBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const Parameters& p
)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
}

void SuctionInletBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      BC["type"]=OFDictData::data("pressureInletOutletVelocity");
      BC["value"]=OFDictData::data("uniform ( 0 0 0 )");
    }
    else if ( 
      ( (field.first=="p") || (field.first=="pd") || (field.first=="p_rgh") )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("totalPressure");
      BC["p0"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.pressure()));
      BC["U"]=OFDictData::data(p_.UName());
      BC["phi"]=OFDictData::data(p_.phiName());
      BC["rho"]=OFDictData::data(p_.rhoName());
      BC["psi"]=OFDictData::data(p_.psiName());
      BC["gamma"]=OFDictData::data(p_.gamma());
      BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.pressure()));
    }
    else if ( (field.first=="rho") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
    }
    else if 
    ( 
      (
	(field.first=="k") ||
	(field.first=="epsilon") ||
	(field.first=="omega") ||
	(field.first=="nut") ||
	(field.first=="nuSgs") ||
	(field.first=="nuTilda")
      )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    else
    {
      if (!(
	  noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
	  ||
	  p_.phasefractions()->addIntoFieldDictionary(field.first, field.second, BC)
	  ))
	//throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
      {
	BC["type"]=OFDictData::data("zeroGradient");
      }
    }
  }
}

VelocityInletBC::VelocityInletBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const Parameters& p
)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
}

void VelocityInletBC::setField_U(OFDictData::dict& BC) const
{
  BC["type"]=OFDictData::data("fixedValue");
  BC["value"]=OFDictData::data("uniform ( "
    +lexical_cast<std::string>(p_.velocity()(0))+" "
    +lexical_cast<std::string>(p_.velocity()(1))+" "
    +lexical_cast<std::string>(p_.velocity()(2))+" )");
}

void VelocityInletBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      setField_U(BC);
    }
    
    else if ( 
      (field.first=="p") && (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    
    else if ( 
      ( (field.first=="pd") || (field.first=="p_rgh") )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      if (OFversion()>=210)
	BC["type"]=OFDictData::data("fixedFluxPressure");
      else
	BC["type"]=OFDictData::data("buoyantPressure");
//       BC["type"]=OFDictData::data("calculated");
//       BC["value"]=OFDictData::data("uniform 0");
    }
    
    else if ( (field.first=="rho") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
    }
    else if ( (field.first=="k") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]="uniform 1e-10";
    }
    else if ( (field.first=="omega") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]="uniform 1.0";
    }
    else if ( (field.first=="nut") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]="uniform 1e-10";
    }
    else if ( (field.first=="nuSgs") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]="uniform 1e-10";
    }
    else
    {
      if (!(
	  noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
	  ||
	  p_.phasefractions()->addIntoFieldDictionary(field.first, field.second, BC)
	  ))
	{
	  BC["type"]=OFDictData::data("zeroGradient");
	}
	//throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
    }
  }
}

TurbulentVelocityInletBC::TurbulentVelocityInletBC
(
  OpenFOAMCase& c,
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  Parameters const& p
)
: VelocityInletBC(c, patchName, boundaryDict, p),
  p_(p)
{
}


void TurbulentVelocityInletBC::setField_U(OFDictData::dict& BC) const
{
  std::string Uvec="( "
    +lexical_cast<std::string>(p_.velocity()(0))+" "
    +lexical_cast<std::string>(p_.velocity()(1))+" "
    +lexical_cast<std::string>(p_.velocity()(2))+" )";
    
  BC["type"]="inflowGenerator<"+p_.structureType()+">";
  BC["Umean"]="uniform "+Uvec;
  
  BC["c"]="uniform 16";
  double L=p_.mixingLength();
  BC["L"]="uniform ( "
    +lexical_cast<string>(L)+" 0 0 "
    +lexical_cast<string>(L)+" 0 "
    +lexical_cast<string>(L)+" )";

  double R=pow(p_.turbulenceIntensity()*norm(p_.velocity(),2), 2);
  BC["R"]="uniform ( "
    +lexical_cast<string>(R)+" 0 0 "
    +lexical_cast<string>(R)+" 0 "
    +lexical_cast<string>(R)+" )";

  BC["value"]="uniform "+Uvec;
}

void TurbulentVelocityInletBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  VelocityInletBC::addIntoFieldDictionaries(dictionaries);
  
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict.addListIfNonexistent("libs").push_back( OFDictData::data("\"libinflowGeneratorBC.so\"") );
}

void TurbulentVelocityInletBC::initInflowBC(const boost::filesystem::path& location) const
{
  OFDictData::dictFile inflowProperties;
  
  OFDictData::list& initializers = inflowProperties.addListIfNonexistent("initializers");
  
  OFDictData::dict d;
  d["Ubulk"]=arma::norm(p_.velocity(), 2);
  d["patchName"]=patchName_;
  
  OFDictData::dict d_long, d_lat;
  
  const TurbulentVelocityInletBC::CoeffList* cl
   = boost::get<TurbulentVelocityInletBC::CoeffList>( &(p_.longLengthScale()) );
  if (cl)
  {
    d_long["c0"]=(*cl)(0);
    d_long["c1"]=(*cl)(1);
    d_long["c2"]=(*cl)(2);
    d_long["c3"]=(*cl)(3);
  }
  else
  {
    cout<<"Using default coefficients for longitudinal length scale profile"<<endl;
    d_long["c0"]=0.78102065;
    d_long["c1"]=-0.30801496;
    d_long["c2"]=0.18299657;
    d_long["c3"]=3.73012118;
  }
  d["L_long"]=d_long;
  
  cl = boost::get<TurbulentVelocityInletBC::CoeffList>( &(p_.latLengthScale()) );
  if (cl)
  {
    d_lat["c0"]=(*cl)(0);
    d_lat["c1"]=(*cl)(1);
    d_lat["c2"]=(*cl)(2);
    d_lat["c3"]=(*cl)(3);
  }
  else
  {
    cout<<"Using default coefficients for lateral length scale profile"<<endl;
    d_lat["c0"]=0.84107675;
    d_lat["c1"]=-0.63386837;
    d_lat["c2"]=0.62172817;
    d_lat["c3"]=0.7780003;
  }
  d["L_lat"]=d_lat;

  initializers.push_back( p_.initializerType() );
  if (p_.initializerType()=="pipeFlow")
  {
    d["D"]=2.0*p_.delta();
  }
  else if (p_.initializerType()=="channelFlow")
  {
    d["H"]=2.0*p_.delta();
  }
  initializers.push_back( d );
  
  // then write to file
  inflowProperties.write( location / "constant" / "inflowProperties" );
  
  OFcase().executeCommand(location, "initInflowGenerator");
}
  
PressureOutletBC::PressureOutletBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const Parameters& p
)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
}

void PressureOutletBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      BC["type"]=OFDictData::data("inletOutlet");
      BC["inletValue"]=OFDictData::data("uniform ( 0 0 0 )");
      BC["value"]=OFDictData::data("uniform ( 0 0 0 )");
    }
    else if ( 
      ( (field.first=="p") || (field.first=="pd") || (field.first=="p_rgh") )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.pressure()));
    }
    else if ( (field.first=="rho") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
    }
    else if 
    (
      (
	(field.first=="k") ||
	(field.first=="epsilon") ||
	(field.first=="omega") ||
	(field.first=="nut") ||
	(field.first=="nuSgs") ||
	(field.first=="nuTilda")
      )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    else
    {
      if (!(
	  noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
/*	  ||
	  p_.phasefractions()->addIntoFieldDictionary(field.first, field.second, BC)*/
	  ))
	//throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
	{
	  BC["type"]=OFDictData::data("zeroGradient");
	}
	  
    }
  }
}


MeshMotionBC::MeshMotionBC()
{
}

MeshMotionBC::~MeshMotionBC()
{
}

void MeshMotionBC::addIntoDictionaries(OFdicts& dictionaries) const
{}

bool NoMeshMotion::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
    if 
    ( 
      ((fieldname=="displacement")||(fieldname == "motionU"))
      && 
      (get<0>(fieldinfo)==vectorField) 
    )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform (0 0 0)");
      return true;
    }
    else 
      return false;
}

MeshMotionBC* NoMeshMotion::clone() const
{
  return new NoMeshMotion(*this);
}

NoMeshMotion noMeshMotion;



CAFSIBC::CAFSIBC(const Parameters& p)
: p_(p)
{
}

CAFSIBC::~CAFSIBC()
{
}

void CAFSIBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict.getList("libs").push_back( OFDictData::data("\"libFEMDisplacementBC.so\"") );
}

bool CAFSIBC::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (fieldname == "motionU")
  {
    BC["prescribeMotionVelocity"] = OFDictData::data(true);
  }
  if ( (fieldname == "pointDisplacement") || (fieldname == "motionU") )
  {
    BC["type"]= OFDictData::data("FEMDisplacement");
    BC["FEMCaseDir"]=  OFDictData::data(std::string("\"")+p_.FEMScratchDir().c_str()+"\"");
    BC["pressureScale"]=  OFDictData::data(p_.pressureScale());
    BC["minPressure"]=  OFDictData::data(p_.clipPressure());
    BC["nSmoothIter"]=  OFDictData::data(4);
    BC["wallCollisionCheck"]=  OFDictData::data(true);
    if (p_.oldPressure().get())
    {
      std::ostringstream oss;
      oss<<"uniform "<<*p_.oldPressure();
      BC["oldPressure"] = OFDictData::data(oss.str());
    }
    BC["value"]=OFDictData::data("uniform (0 0 0)");
    
    OFDictData::list relaxProfile;
    if (p_.relax().which()==0)
    {
      OFDictData::list cp;
      cp.push_back(0.0);
      cp.push_back( boost::get<double>(p_.relax()) );
      relaxProfile.push_back( cp );
    }
    else
    {
      BOOST_FOREACH(const RelaxProfile::value_type& rp,  boost::get<RelaxProfile>(p_.relax()) )
      {
	OFDictData::list cp;
	cp.push_back(rp.first);
	cp.push_back(rp.second);
	relaxProfile.push_back(cp);
      }
    }
    BC["relax"]=  relaxProfile;
    
    return true;
  }
  return false;
}

MeshMotionBC* CAFSIBC::clone() const
{
  return new CAFSIBC(*this);
}


WallBC::WallBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, Parameters const& p)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
  type_="wall";
}

void WallBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  p_.meshmotion()->addIntoDictionaries(dictionaries);
  BoundaryCondition::addIntoDictionaries(dictionaries);
}

void WallBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC = dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    
    // velocity
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform ("+toStr(p_.wallVelocity())+")");
    }
    
    // pressure
    else if ( (field.first=="p") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
        
    // pressure
    else if ( ( (field.first=="p_rgh") || (field.first=="pd") ) 
	      && (get<0>(field.second)==scalarField) )
    {
      if (OFversion()>=220)
	BC["type"]=OFDictData::data("fixedFluxPressure");
      else
	BC["type"]=OFDictData::data("buoyantPressure");
//       BC["type"]=OFDictData::data("buoyantPressure");
    }
    
    // turbulence quantities, should be handled by turbulence model
    else if ( 
      ( (field.first=="k") || (field.first=="omega") || (field.first=="nut") ) 
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      OFcase().get<turbulenceModel>("turbulenceModel")->addIntoFieldDictionary(field.first, field.second, BC);
    }
    
    // any other scalar field
    else if (get<0>(field.second)==scalarField)
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    
    else
    {
      if (!p_.meshmotion()->addIntoFieldDictionary(field.first, field.second, BC))
	//throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
	{
	  BC["type"]=OFDictData::data("zeroGradient");
	}
    }
  }
}

void WallBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  BoundaryCondition::addOptionsToBoundaryDict(bndDict);
}

}

