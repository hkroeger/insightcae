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
 *
 */


#include "basiccaseelements.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"

#include <utility>
#include "boost/assign.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/format.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;

namespace insight
{

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


volumeDrag::volumeDrag(OpenFOAMCase& c, const volumeDrag::Parameters& p)
: OpenFOAMCaseElement(c, p.name()),
  p_(p)
{}


void volumeDrag::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");  
  controlDict.getList("libs").insertNoDuplicate( "\"libvolumeDragfvOption.so\"" );  
  
  OFDictData::dict cd;
  cd["type"]="volumeDrag";
  cd["active"]=true;
  cd["selectionMode"]="cellZone";
  cd["cellZone"]=p_.name();
  OFDictData::dict vdd;
  vdd["CD"]=OFDictData::to_OF(p_.CD());
  cd["volumeDragCoeffs"]=vdd;
  
  OFDictData::dict& fvOptions=dictionaries.addDictionaryIfNonexistent("system/fvOptions");
  fvOptions[p_.name()]=cd;     
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
  OFDictData::list nrp; nrp.resize(p_.nonRotatingPatches().size());
  copy(p_.nonRotatingPatches().begin(), p_.nonRotatingPatches().end(), nrp.begin());
  
  if (OFversion()<220)
  {
    OFDictData::dict coeffs;
    coeffs["nonRotatingPatches"]=nrp;
    coeffs["origin"]=OFDictData::dimensionedData(
      "origin", dimLength, OFDictData::vector3(p_.rotationCentre())
    );
    coeffs["axis"]=OFDictData::dimensionedData(
      "axis", dimless, OFDictData::vector3(p_.rotationAxis())
    );
    coeffs["omega"]=OFDictData::dimensionedData(
      "omega", OFDictData::dimension(0, 0, -1, 0, 0, 0, 0), 
      2.*M_PI*p_.rpm()/60.
    );

    OFDictData::dict& MRFZones=dictionaries.addDictionaryIfNonexistent("constant/MRFZones");
    OFDictData::list& MRFZoneList = MRFZones.addListIfNonexistent("");     
    MRFZoneList.push_back(p_.name());
    MRFZoneList.push_back(coeffs);
    
    OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
    if (controlDict.find("application")!=controlDict.end())
      if (controlDict.getString("application")=="simpleFoam")
	controlDict["application"]="MRFSimpleFoam";
  }
  else
  {
    OFDictData::dict coeffs;
    
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
  
  if (OFversion()<230)
  {
    OFDictData::dict& twoPhase=transportProperties.addSubDictIfNonexistent("twoPhase");
    twoPhase["transportModel"]="twoPhase";
    twoPhase["phase1"]="phase1";
    twoPhase["phase2"]="phase2";
  } else
  {
    OFDictData::list& pl=transportProperties.addListIfNonexistent("phases");
    pl.push_back("phase1");
    pl.push_back("phase2");
  }
  
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
  LESProperties["printCoeffs"]=true;

  LESProperties["LESModel"]="oneEqEddy";
  //LESProperties["delta"]="cubeRootVol";
  LESProperties["delta"]="vanDriest";
  
  OFDictData::dict crvc;
  crvc["deltaCoeff"]=1.0;
  LESProperties["cubeRootVolCoeffs"]=crvc;
  
  OFDictData::dict vdc;
  vdc["deltaCoeff"]=1.0;
  vdc["delta"]="cubeRootVol";
  vdc["cubeRootVolCoeffs"]=crvc;
  LESProperties["vanDriestCoeffs"]=vdc;
  
  LESProperties.addSubDictIfNonexistent("laminarCoeffs");
}

bool oneEqEddy_LESModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (fieldname == "k")
  {
    BC["type"]="fixedValue";
    BC["value"]="uniform 1e-10";
    return true;
  }
  else if (fieldname == "nuSgs")
  {
    BC["type"]="zeroGradient";
    return true;
  }
  
  return false;
}

defineType(dynOneEqEddy_LESModel);
addToFactoryTable(turbulenceModel, dynOneEqEddy_LESModel, turbulenceModel::ConstrP);

void dynOneEqEddy_LESModel::addFields()
{
  OFcase().addField("k", 	FieldInfo(scalarField, 	dimKinEnergy, 	list_of(1e-10), volField ) );
  OFcase().addField("nuSgs", 	FieldInfo(scalarField, 	dimKinViscosity, 	list_of(1e-10), volField ) );
}
  

dynOneEqEddy_LESModel::dynOneEqEddy_LESModel(OpenFOAMCase& c)
: LESModel(c)
{
  addFields();
}

dynOneEqEddy_LESModel::dynOneEqEddy_LESModel(const ConstrP& c)
: LESModel(c)
{
  addFields();
}

void dynOneEqEddy_LESModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  LESModel::addIntoDictionaries(dictionaries);
  
  OFDictData::dict& LESProperties=dictionaries.addDictionaryIfNonexistent("constant/LESProperties");
  LESProperties["printCoeffs"]=true;

  LESProperties["LESModel"]="dynOneEqEddy";
  LESProperties["delta"]="cubeRootVol";
//   LESProperties["delta"]="vanDriest";
  
  OFDictData::dict doeec;
  doeec["filter"]="simple";
  LESProperties["dynOneEqEddyCoeffs"]=doeec;
  
  OFDictData::dict crvc;
  crvc["deltaCoeff"]=1.0;
  LESProperties["cubeRootVolCoeffs"]=crvc;
  
  OFDictData::dict vdc;
  vdc["deltaCoeff"]=1.0;
  vdc["delta"]="cubeRootVol";
  vdc["cubeRootVolCoeffs"]=crvc;
  LESProperties["vanDriestCoeffs"]=vdc;
  
  LESProperties.addSubDictIfNonexistent("laminarCoeffs");
}

bool dynOneEqEddy_LESModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (fieldname == "k")
  {
    BC["type"]="fixedValue";
    BC["value"]="uniform 1e-10";
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
  //LESProperties["delta"]="cubeRootVol";
  LESProperties["delta"]="vanDriest";
  LESProperties["printCoeffs"]=true;
  
  OFDictData::dict crvc;
  crvc["deltaCoeff"]=1.0;
  LESProperties["cubeRootVolCoeffs"]=crvc;

  OFDictData::dict vdc;
  vdc["deltaCoeff"]=1.0;
  vdc["delta"]="cubeRootVol";
  vdc["cubeRootVolCoeffs"]=crvc;
  LESProperties["vanDriestCoeffs"]=vdc;

  OFDictData::dict& cd=LESProperties.addSubDictIfNonexistent(modelName+"Coeffs");
  cd["filter"]="simple";
}

bool dynSmagorinsky_LESModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (fieldname == "k")
  {
    BC["type"]="fixedValue";
    BC["value"]="uniform 1e-10";
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
  if (OFcase().isCompressible())
  {
    OFcase().addField("mut", 	FieldInfo(scalarField, 	dimDynViscosity, 	list_of(1e-10), volField ) );
    OFcase().addField("alphat", 	FieldInfo(scalarField, 	dimDynViscosity, 	list_of(1e-10), volField ) );
  }
  else
  {
    OFcase().addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	list_of(1e-10), volField ) );
  }
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
  std::string pref="";
  if (OFcase().isCompressible()) pref="compressible::";
  
  if (fieldname == "k")
  {
    BC["type"]=OFDictData::data(pref+"kqRWallFunction");
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  else if (fieldname == "omega")
  {
    BC["type"]=OFDictData::data(pref+"omegaWallFunction");
    BC["Cmu"]=0.09;
    BC["kappa"]=0.41;
    BC["E"]=9.8;
    BC["beta1"]=0.075;
    BC["value"]="uniform 1";
    return true;
  }
  else if (fieldname == "nut")
  {
//     BC["type"]=OFDictData::data("nutkWallFunction");
    BC["type"]=OFDictData::data("nutUWallFunction"); // buggy in OF16ext/Fx31?
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  else if (fieldname == "mut")
  {
    BC["type"]=OFDictData::data("mutkWallFunction");
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  else if (fieldname == "alphat")
  {
    BC["type"]=OFDictData::data(pref+"alphatWallFunction");
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  
  return false;
}

void kEpsilonBase_RASModel::addFields()
{
  OFcase().addField("k", 	FieldInfo(scalarField, 	dimKinEnergy, 	list_of(1e-10), volField ) );
  OFcase().addField("epsilon", 	FieldInfo(scalarField, 	OFDictData::dimension(0, 2, -3), 	list_of(10.0), volField ) );
  if (OFcase().isCompressible())
  {
    OFcase().addField("mut", 	FieldInfo(scalarField, 	dimDynViscosity, 	list_of(1e-10), volField ) );
    OFcase().addField("alphat", 	FieldInfo(scalarField, 	dimDynViscosity, 	list_of(1e-10), volField ) );
  }
  else
  {
    OFcase().addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	list_of(1e-10), volField ) );
  }
}

kEpsilonBase_RASModel::kEpsilonBase_RASModel(OpenFOAMCase& c)
: RASModel(c)
{
  addFields();
}
  
kEpsilonBase_RASModel::kEpsilonBase_RASModel(const turbulenceModel::ConstrP& c)
: RASModel(c)
{
  addFields();
}
  
void kEpsilonBase_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=dictionaries.addDictionaryIfNonexistent("constant/RASProperties");
  RASProperties["RASModel"]=this->type(); //"kEpsilon";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties["kMin"]=1e-3;
  RASProperties["epsilonMin"]=1e-3;
  RASProperties.addSubDictIfNonexistent(type()+"Coeffs");
}

bool kEpsilonBase_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  std::string pref="";
  if (OFcase().isCompressible()) pref="compressible::";
  
  if (fieldname == "k")
  {
    BC["type"]=OFDictData::data(pref+"kqRWallFunction");
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  else if (fieldname == "epsilon")
  {
    BC["type"]=OFDictData::data(pref+"epsilonWallFunction");
//     BC["Cmu"]=0.09;
//     BC["kappa"]=0.41;
//     BC["E"]=9.8;
//     BC["beta1"]=0.075;
    BC["value"]="uniform 10";
    return true;
  }
  else if (fieldname == "nut")
  {
    BC["type"]=OFDictData::data("nutkWallFunction");
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  else if (fieldname == "mut")
  {
    BC["type"]=OFDictData::data("mutkWallFunction");
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  else if (fieldname == "alphat")
  {
    BC["type"]=OFDictData::data(pref+"alphatWallFunction");
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  
  return false;
}


defineType(kEpsilon_RASModel);
addToFactoryTable(turbulenceModel, kEpsilon_RASModel, turbulenceModel::ConstrP);
kEpsilon_RASModel::kEpsilon_RASModel(const turbulenceModel::ConstrP& c): kEpsilonBase_RASModel(c) {}


defineType(realizablekEpsilon_RASModel);
addToFactoryTable(turbulenceModel, realizablekEpsilon_RASModel, turbulenceModel::ConstrP);
realizablekEpsilon_RASModel::realizablekEpsilon_RASModel(const turbulenceModel::ConstrP& c): kEpsilonBase_RASModel(c) {}


defineType(SpalartAllmaras_RASModel);
addToFactoryTable(turbulenceModel, SpalartAllmaras_RASModel, turbulenceModel::ConstrP);

void SpalartAllmaras_RASModel::addFields()
{
  OFcase().addField("nuTilda", 	FieldInfo(scalarField, 	dimKinViscosity, 	list_of(1e-10), volField ) );
  if (OFcase().isCompressible())
  {
    OFcase().addField("mut", 	FieldInfo(scalarField, 	dimDynViscosity, 	list_of(1e-10), volField ) );
    OFcase().addField("alphat", 	FieldInfo(scalarField, 	dimDynViscosity, 	list_of(1e-10), volField ) );
  }
  else
  {
    OFcase().addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	list_of(1e-10), volField ) );
  }
}

SpalartAllmaras_RASModel::SpalartAllmaras_RASModel(OpenFOAMCase& c)
: RASModel(c)
{
  addFields();
}
  
SpalartAllmaras_RASModel::SpalartAllmaras_RASModel(const turbulenceModel::ConstrP& c)
: RASModel(c)
{
  addFields();
}
  
void SpalartAllmaras_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=dictionaries.addDictionaryIfNonexistent("constant/RASProperties");
  RASProperties["RASModel"]="SpalartAllmaras";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.addSubDictIfNonexistent("SpalartAllmarasCoeffs");
}

bool SpalartAllmaras_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
//   std::string pref="";
//   if (OFcase().isCompressible()) pref="compressible::";
  
  if (fieldname == "nuTilda")
  {
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]=OFDictData::data("uniform 0");
    return true;
  }
  else if (fieldname == "nut")
  {
    BC["type"]=OFDictData::data("nutUSpaldingWallFunction");
    BC["value"]=OFDictData::data("uniform 0");
    return true;
  }
//   else if (fieldname == "mut")
//   {
//     BC["type"]=OFDictData::data("mutkWallFunction");
//     BC["value"]=OFDictData::data("uniform 1e-10");
//     return true;
//   }
//   else if (fieldname == "alphat")
//   {
//     BC["type"]=OFDictData::data(pref+"alphatWallFunction");
//     BC["value"]=OFDictData::data("uniform 1e-10");
//     return true;
//   }
  
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
  if (fieldname == "k")
  {
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]="uniform "+str(format("%g") % 1e-10);
    return true;
  }
  else if ( fieldname == "omega")
  {
    BC["type"]=OFDictData::data("omegaWallFunction");
    BC["value"]="uniform "+str(format("%g") % 1e-10);
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

defineType(kOmegaHe_RASModel);
addToFactoryTable(turbulenceModel, kOmegaHe_RASModel, turbulenceModel::ConstrP);

kOmegaHe_RASModel::kOmegaHe_RASModel(OpenFOAMCase& c)
: kOmegaSST_RASModel(c)
{}
  
kOmegaHe_RASModel::kOmegaHe_RASModel(const turbulenceModel::ConstrP& c)
: kOmegaSST_RASModel(c)
{}
  
void kOmegaHe_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict.getList("libs").push_back( OFDictData::data("\"libkOmegaHe.so\"") );

  OFDictData::dict& RASProperties=dictionaries.addDictionaryIfNonexistent("constant/RASProperties");
  RASProperties["RASModel"]="kOmegaHe";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.addSubDictIfNonexistent("kOmegaHeCoeffs");

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  div["div(nonlinear)"]="Gauss linear"; //pref+"Gauss upwind";
  div["div((nuEff*T(grad(U))))"]="Gauss linear";

}

bool kOmegaHe_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (fieldname == "k")
  {
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]="uniform "+str(format("%g") % 1e-10);
    return true;
  }
  else if ( fieldname == "omega")
  {
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]="uniform "+str(format("%g") % 1e-10);
    return true;
  }
  else if ( fieldname == "nut")
  {
    BC["type"]=OFDictData::data("calculated");
    BC["value"]="uniform "+str(format("%g") % 1e-10);
    return true;
  }
  return false;
}

defineType(LRR_RASModel);
addToFactoryTable(turbulenceModel, LRR_RASModel, turbulenceModel::ConstrP);

void LRR_RASModel::addFields()
{
  OFcase().addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 			list_of(1e-10), volField ) );
  OFcase().addField("k", 	FieldInfo(scalarField, 	dimKinEnergy, 				list_of(1e-10), volField ) );
  OFcase().addField("epsilon", 	FieldInfo(scalarField, 	OFDictData::dimension(0, 2, -3), 	list_of(10.0), volField ) );
  OFcase().addField("R", 	FieldInfo(symmTensorField, OFDictData::dimension(0, 2, -2), 	list_of(1e-10)(1e-10)(1e-10)(1e-10)(1e-10)(1e-10), volField ) );
}

LRR_RASModel::LRR_RASModel(OpenFOAMCase& c)
: RASModel(c)
{
  addFields();
}
  
LRR_RASModel::LRR_RASModel(const turbulenceModel::ConstrP& c)
: RASModel(c)
{
  addFields();
}
  
void LRR_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=dictionaries.addDictionaryIfNonexistent("constant/RASProperties");
  RASProperties["RASModel"]="LRR";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.addSubDictIfNonexistent("LRRCoeffs");
}

bool LRR_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
//   std::string pref="";
//   if (OFcase().isCompressible()) pref="compressible::";
  
  if (fieldname == "k")
  {
    BC["type"]=OFDictData::data("kqRWallFunction");
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  else if (fieldname == "epsilon")
  {
    BC["type"]=OFDictData::data("epsilonWallFunction");
//     BC["Cmu"]=0.09;
//     BC["kappa"]=0.41;
//     BC["E"]=9.8;
//     BC["beta1"]=0.075;
    BC["value"]="uniform 10";
    return true;
  }
  else if (fieldname == "nut")
  {
    BC["type"]=OFDictData::data("nutkWallFunction");
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  else if (fieldname == "R")
  {
    BC["type"]=OFDictData::data("kqRWallFunction");
    BC["value"]=OFDictData::data("uniform (1e-10 1e-10 1e-10 1e-10 1e-10 1e-10)");
    return true;
  }
//   else if (fieldname == "mut")
//   {
//     BC["type"]=OFDictData::data("mutkWallFunction");
//     BC["value"]=OFDictData::data("uniform 1e-10");
//     return true;
//   }
//   else if (fieldname == "alphat")
//   {
//     BC["type"]=OFDictData::data(pref+"alphatWallFunction");
//     BC["value"]=OFDictData::data("uniform 1e-10");
//     return true;
//   }
  
  return false;
}

}


