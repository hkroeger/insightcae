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

    
    
    
defineType(gravity);
addToOpenFOAMCaseElementFactoryTable(gravity);

gravity::gravity( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "gravity"),
  p_(ps)
{
}

void gravity::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& g=dictionaries.addDictionaryIfNonexistent("constant/g");
  g["dimensions"]="[0 1 -2 0 0 0 0]";
  OFDictData::list gv;
  for (int i=0; i<3; i++) gv.push_back(p_.g(i));
  g["value"]=gv;
}



defineType(volumeDrag);
addToOpenFOAMCaseElementFactoryTable(volumeDrag);

volumeDrag::volumeDrag( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, ""),
  p_(ps)
{
    name_="volumeDrag"+p_.name;
}


void volumeDrag::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");  
  controlDict.getList("libs").insertNoDuplicate( "\"libvolumeDragfvOption.so\"" );  
  
  OFDictData::dict cd;
  cd["type"]="volumeDrag";
  cd["active"]=true;
  cd["selectionMode"]="cellZone";
  cd["cellZone"]=p_.name;
  OFDictData::dict vdd;
  vdd["CD"]=OFDictData::to_OF(p_.CD);
  cd["volumeDragCoeffs"]=vdd;
  
  OFDictData::dict& fvOptions=dictionaries.addDictionaryIfNonexistent("system/fvOptions");
  fvOptions[p_.name]=cd;     
}


  
transportModel::transportModel(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "transportModel")
{
}




defineType(MRFZone);
addToOpenFOAMCaseElementFactoryTable(MRFZone);

MRFZone::MRFZone( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, ""),
  p_(ps)
{
    name_="MRFZone"+p_.name;
}

void MRFZone::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::list nrp; nrp.resize(p_.nonRotatingPatches.size());
  copy(p_.nonRotatingPatches.begin(), p_.nonRotatingPatches.end(), nrp.begin());
  
  if (OFversion()<220)
  {
    OFDictData::dict coeffs;
    coeffs["nonRotatingPatches"]=nrp;
    coeffs["origin"]=OFDictData::dimensionedData(
      "origin", dimLength, OFDictData::vector3(p_.rotationCentre)
    );
    coeffs["axis"]=OFDictData::dimensionedData(
      "axis", dimless, OFDictData::vector3(p_.rotationAxis)
    );
    coeffs["omega"]=OFDictData::dimensionedData(
      "omega", OFDictData::dimension(0, 0, -1, 0, 0, 0, 0), 
      2.*M_PI*p_.rpm/60.
    );

    OFDictData::dict& MRFZones=dictionaries.addDictionaryIfNonexistent("constant/MRFZones");
    OFDictData::list& MRFZoneList = MRFZones.addListIfNonexistent("");     
    MRFZoneList.push_back(p_.name);
    MRFZoneList.push_back(coeffs);
    
    OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
    if (controlDict.find("application")!=controlDict.end())
      if (controlDict.getString("application")=="simpleFoam")
	controlDict["application"]="MRFSimpleFoam";
  }
  else if (OFversion()>=300)
  {
    OFDictData::dict fod;

    fod["nonRotatingPatches"]=nrp;
    fod["origin"]=OFDictData::vector3(p_.rotationCentre);
    fod["axis"]=OFDictData::vector3(p_.rotationAxis);
    fod["omega"]=2.*M_PI*p_.rpm/60.;

    fod["active"]=true;
    fod["selectionMode"]="cellZone";
    fod["cellZone"]=p_.name;

    OFDictData::dict& MRFProps=dictionaries.addDictionaryIfNonexistent("constant/MRFProperties");
    MRFProps[p_.name]=fod;

  }
  else
  {
    OFDictData::dict coeffs;
    
    coeffs["nonRotatingPatches"]=nrp;
    coeffs["origin"]=OFDictData::vector3(p_.rotationCentre);
    coeffs["axis"]=OFDictData::vector3(p_.rotationAxis);
    coeffs["omega"]=2.*M_PI*p_.rpm/60.;

    OFDictData::dict fod;
    fod["type"]="MRFSource";
    fod["active"]=true;
    fod["selectionMode"]="cellZone";
    fod["cellZone"]=p_.name;
    fod["MRFSourceCoeffs"]=coeffs;
    
    OFDictData::dict& fvOptions=dictionaries.addDictionaryIfNonexistent("system/fvOptions");
    fvOptions[p_.name]=fod;     
  }
}



defineType(PassiveScalar);
addToOpenFOAMCaseElementFactoryTable(PassiveScalar);

PassiveScalar::PassiveScalar( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "PassiveScalar"),
  p_(ps)
{
}

void PassiveScalar::addFields( OpenFOAMCase& c ) const
{
  c.addField(p_.fieldname, 	FieldInfo(scalarField, 	dimless, 	list_of(0.0), volField ) );
}


void PassiveScalar::addIntoDictionaries(OFdicts& dictionaries) const
{  
    OFDictData::dict Fd;
    Fd["type"]="scalarTransport";
    Fd["field"]=p_.fieldname;
    Fd["resetOnStartUp"]=false;
    Fd["autoSchemes"]=false;
    Fd["fvOptions"]=OFDictData::dict();
    
    OFDictData::list fol;
    fol.push_back("\"libutilityFunctionObjects.so\"");
    Fd["functionObjectLibs"]=fol;
    
    
    OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
    controlDict.addSubDictIfNonexistent("functions")[p_.fieldname+"_transport"]=Fd;
    
    
    OFDictData::dict& fvSchemes=dictionaries.addDictionaryIfNonexistent("system/fvSchemes");
    OFDictData::dict& divSchemes = fvSchemes.addSubDictIfNonexistent("divSchemes");
    divSchemes["div(phi,"+p_.fieldname+")"]="Gauss limitedLinear01 1";

    OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
    OFDictData::dict& solvers=fvSolution.subDict("solvers");
    solvers[p_.fieldname]=smoothSolverSetup(1e-6, 0.);

}



defineType(PressureGradientSource);
addToOpenFOAMCaseElementFactoryTable(PressureGradientSource);

PressureGradientSource::PressureGradientSource( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "PressureGradientSource"),
  p_(ps)
{
}

void PressureGradientSource::addIntoDictionaries(OFdicts& dictionaries) const
{
  if (OFversion()>=220)
  {
    OFDictData::dict coeffs;    
    OFDictData::list flds; flds.push_back("U");
    coeffs["Ubar"]=OFDictData::vector3(p_.Ubar);

    OFDictData::dict fod;
    if (OFversion()>=300)
    {
        fod["type"]="meanVelocityForce";
        coeffs["selectionMode"]="all";
        if (OFversion()>=400)
        {
            coeffs["fields"]=flds;
        }
        else
        {
            coeffs["fieldNames"]=flds;
        }
        fod["meanVelocityForceCoeffs"]=coeffs;
    }
    else
    {
        fod["type"]="pressureGradientExplicitSource";
        fod["selectionMode"]="all";
        coeffs["fieldNames"]=flds;
        fod["pressureGradientExplicitSourceCoeffs"]=coeffs;
    }
    fod["active"]=true;
    
    OFDictData::dict& fvOptions=dictionaries.addDictionaryIfNonexistent("system/fvOptions");
    fvOptions[name()]=fod;  
  }
  else
  {
    // for channelFoam:
    OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
    transportProperties["Ubar"]=OFDictData::dimensionedData("Ubar", dimVelocity, OFDictData::vector3(p_.Ubar));
  }
}



defineType(ConstantPressureGradientSource);
addToOpenFOAMCaseElementFactoryTable(ConstantPressureGradientSource);

ConstantPressureGradientSource::ConstantPressureGradientSource( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "ConstantPressureGradientSource"),
  p_(ps)
{
}

void ConstantPressureGradientSource::addIntoDictionaries(OFdicts& dictionaries) const
{
  if (OFversion()>=230)
  {
    OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");  
    controlDict.getList("libs").insertNoDuplicate( "\"libconstantPressureGradient.so\"" );  

    OFDictData::dict coeffs;
    OFDictData::list flds; flds.push_back("U");
    coeffs["fieldNames"]=flds;
    coeffs["gradP"]=OFDictData::dimensionedData("gradP", OFDictData::dimension(0, 1, -2, 0, 0, 0, 0), OFDictData::vector3(p_.gradp));

    OFDictData::dict fod;
    fod["type"]="constantPressureGradientExplicitSource";
    fod["active"]=true;
    fod["selectionMode"]="all";
    fod["constantPressureGradientExplicitSourceCoeffs"]=coeffs;

    OFDictData::dict& fvOptions=dictionaries.addDictionaryIfNonexistent("system/fvOptions");
    fvOptions[name()]=fod;
  }
  else
  {
    throw insight::Exception("constantPressureGradient unavailable!");
    // for channelFoam:
 //   OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
 //   transportProperties["Ubar"]=OFDictData::dimensionedData("Ubar", dimVelocity, OFDictData::vector3(p_.Ubar()));
  }
}





defineType(singlePhaseTransportProperties);
addToOpenFOAMCaseElementFactoryTable(singlePhaseTransportProperties);

singlePhaseTransportProperties::singlePhaseTransportProperties( OpenFOAMCase& c, const ParameterSet& ps )
: transportModel(c),
  p_(ps)
{
}
 
void singlePhaseTransportProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
  transportProperties["transportModel"]="Newtonian";
  transportProperties["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimension(0, 2, -1), p_.nu);
}





defineType(perfectGasSinglePhaseThermophysicalProperties);
addToOpenFOAMCaseElementFactoryTable(perfectGasSinglePhaseThermophysicalProperties);

perfectGasSinglePhaseThermophysicalProperties::perfectGasSinglePhaseThermophysicalProperties( OpenFOAMCase& c, const ParameterSet& ps )
: transportModel(c),
  p_(ps)
{
}
 
void perfectGasSinglePhaseThermophysicalProperties::addIntoDictionaries(OFdicts& dictionaries) const
{

  OFDictData::dict& thermophysicalProperties=dictionaries.addDictionaryIfNonexistent("constant/thermophysicalProperties");
  
  OFDictData::dict tt;
  tt["type"]="hePsiThermo";
  tt["mixture"]="pureMixture";
  tt["transport"]="const";
  tt["thermo"]="eConst";
  tt["equationOfState"]="perfectGas";
  tt["specie"]="specie";
  tt["energy"]="sensibleInternalEnergy";
  thermophysicalProperties["thermoType"]=tt;
  
  const double R=8.3144598;
  double M = p_.rho*R*p_.Tref/p_.pref;
  double cv = R/(p_.kappa-1.) / M;

  OFDictData::dict mixture, mix_sp, mix_td, mix_tr;
  mix_sp["nMoles"]=1.;
  mix_sp["molWeight"]=1e3*M;
  
  mix_td["Cv"]=cv;
  mix_td["Hf"]=0.;
  
  mix_tr["mu"]=p_.nu*p_.rho;
  mix_tr["Pr"]=p_.Pr;
  
  mixture["specie"]=mix_sp;
  mixture["thermodynamics"]=mix_td;
  mixture["transport"]=mix_tr;
  thermophysicalProperties["mixture"]=mixture;
}




defineType(twoPhaseTransportProperties);
addToOpenFOAMCaseElementFactoryTable(twoPhaseTransportProperties);


twoPhaseTransportProperties::twoPhaseTransportProperties( OpenFOAMCase& c, const ParameterSet& ps )
: transportModel(c),
  p_(ps)
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
  phase1["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimension(0, 2, -1), p_.nu1);
  phase1["rho"]=OFDictData::dimensionedData("rho", OFDictData::dimension(1, -3), p_.rho1);
  
  OFDictData::dict& phase2=transportProperties.addSubDictIfNonexistent("phase2");
  phase2["transportModel"]="Newtonian";
  phase2["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimension(0, 2, -1), p_.nu2);
  phase2["rho"]=OFDictData::dimensionedData("rho", OFDictData::dimension(1, -3), p_.rho2);

  transportProperties["sigma"]=OFDictData::dimensionedData("sigma", OFDictData::dimension(1, 0, -2), p_.sigma);

}

namespace phaseChangeModels
{
    
defineType(phaseChangeModel);
defineFactoryTable(phaseChangeModel, LIST( const ParameterSet& ps ), LIST( ps ) );
defineStaticFunctionTable(phaseChangeModel, defaultParameters, ParameterSet);
  
phaseChangeModel::~phaseChangeModel()
{
}


defineType(SchnerrSauer);
addToFactoryTable(phaseChangeModel, SchnerrSauer);
addToStaticFunctionTable(phaseChangeModel, SchnerrSauer, defaultParameters);

SchnerrSauer::SchnerrSauer(const ParameterSet& p)
: p_(p)
{
}

void SchnerrSauer::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
  transportProperties["phaseChangeTwoPhaseMixture"]="SchnerrSauer";
  
  OFDictData::dict& coeffs=transportProperties.addSubDictIfNonexistent("SchnerrSauerCoeffs");
  coeffs["n"] = OFDictData::dimensionedData("n", OFDictData::dimension(0, -3), p_.n);
  coeffs["dNuc"] = OFDictData::dimensionedData("dNuc", OFDictData::dimension(0, 1), p_.dNuc);
  coeffs["Cc"] = OFDictData::dimensionedData("Cc", OFDictData::dimension(), p_.Cc);
  coeffs["Cv"] = OFDictData::dimensionedData("Cv", OFDictData::dimension(), p_.Cv);
}

}




defineType(cavitationTwoPhaseTransportProperties);
addToOpenFOAMCaseElementFactoryTable(cavitationTwoPhaseTransportProperties);

cavitationTwoPhaseTransportProperties::cavitationTwoPhaseTransportProperties( OpenFOAMCase& c, const ParameterSet& ps )
: twoPhaseTransportProperties(c, ps),
  ps_(ps)
{
}

void cavitationTwoPhaseTransportProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  Parameters p(ps_);   
  twoPhaseTransportProperties::addIntoDictionaries(dictionaries);
  OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
  transportProperties["pSat"]=OFDictData::dimensionedData("pSat", OFDictData::dimension(1, -1, -2), p.psat);
  
  const SelectableSubsetParameter& msp = ps_.get<SelectableSubsetParameter>("model"); 
  phaseChangeModels::phaseChangeModel::lookup(msp.selection(), msp()) ->addIntoDictionaries(dictionaries);
}

ParameterSet cavitationTwoPhaseTransportProperties::defaultParameters()
{
    ParameterSet ps = Parameters::makeDefault();
    
    SelectableSubsetParameter& msp = ps.get<SelectableSubsetParameter>("model");
    for (phaseChangeModels::phaseChangeModel::FactoryTable::const_iterator i = phaseChangeModels::phaseChangeModel::factories_->begin();
        i != phaseChangeModels::phaseChangeModel::factories_->end(); i++)
    {
        ParameterSet defp = phaseChangeModels::phaseChangeModel::defaultParameters(i->first);
        msp.addItem( i->first, defp );
    }
    msp.selection() = phaseChangeModels::phaseChangeModel::factories_->begin()->first;

    return ps;
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




defineType(solidBodyMotionDynamicMesh);
addToOpenFOAMCaseElementFactoryTable(solidBodyMotionDynamicMesh);



solidBodyMotionDynamicMesh::solidBodyMotionDynamicMesh( OpenFOAMCase& c, const ParameterSet& ps )
: dynamicMesh(c),
  ps_(ps)
{
}


void solidBodyMotionDynamicMesh::addIntoDictionaries(OFdicts& dictionaries) const
{
    Parameters p(ps_);
    
    OFDictData::dict& dynamicMeshDict
      = dictionaries.addDictionaryIfNonexistent("constant/dynamicMeshDict");
      
    dynamicMeshDict["dynamicFvMesh"]="dynamicMotionSolverFvMesh";    
    dynamicMeshDict["solver"]="solidBody";
    OFDictData::dict sbc;

    sbc["cellZone"]=p.zonename;

    if ( Parameters::motion_rotation_type* rp = boost::get<Parameters::motion_rotation_type>(&p.motion) )
    {
        sbc["solidBodyMotionFunction"]="rotatingMotion";
        OFDictData::dict rmc;
        rmc["origin"]=OFDictData::vector3(rp->origin);
        rmc["axis"]=OFDictData::vector3(rp->axis);
        rmc["omega"]=2.*M_PI*rp->rpm/60.;
        sbc["rotatingMotionCoeffs"]=rmc;
    }

    dynamicMeshDict["solidBodyCoeffs"]=sbc;
}

}


