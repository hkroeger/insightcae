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


#include "openfoam/caseelements/boundaryconditioncaseelements.h"
#include "openfoam/caseelements/basiccaseelements.h"
#include "openfoam/caseelements/turbulencemodelcaseelements.h"
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
    
    
    


defineType(SimpleBC);
addToFactoryTable(BoundaryCondition, SimpleBC);
addToStaticFunctionTable(BoundaryCondition, SimpleBC, defaultParameters);

void SimpleBC::init()
{
    BCtype_ = p_.className;
    if ( ( OFversion() >=230 ) && ( BCtype_=="symmetryPlane" ) ) {
        BCtype_="symmetry";
    }
}

SimpleBC::SimpleBC ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const std::string className )
    : BoundaryCondition ( c, patchName, boundaryDict )
{
    p_.className = className;
    init();
}

SimpleBC::SimpleBC ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const ParameterSet& p )
    : BoundaryCondition ( c, patchName, boundaryDict ),
      p_ ( p )
{
    init();
}

void SimpleBC::addIntoFieldDictionaries ( OFdicts& dictionaries ) const
{

    BoundaryCondition::addIntoFieldDictionaries ( dictionaries );

    for ( const FieldList::value_type& field: OFcase().fields() ) {
        
        OFDictData::dict& BC=dictionaries.addFieldIfNonexistent ( "0/"+field.first, field.second )
                             .subDict ( "boundaryField" ).subDict ( patchName_ );
                             
        if ( ( BCtype_=="cyclic" ) && ( ( field.first=="motionU" ) || ( field.first=="pointDisplacement" ) ) ) {
            MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC );
        } else {
            std::string tname=BCtype_;
            BC["type"]=OFDictData::data ( tname );
        }
        
    }
}




defineType(SymmetryBC);
addToFactoryTable(BoundaryCondition, SymmetryBC);
addToStaticFunctionTable(BoundaryCondition, SymmetryBC, defaultParameters);


SymmetryBC::SymmetryBC ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const ParameterSet& )
    : SimpleBC ( c, patchName, boundaryDict, "symmetryPlane" )
{}




defineType(EmptyBC);
addToFactoryTable(BoundaryCondition, EmptyBC);
addToStaticFunctionTable(BoundaryCondition, EmptyBC, defaultParameters);


EmptyBC::EmptyBC ( OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const ParameterSet& )
    : SimpleBC ( c, patchName, boundaryDict, "empty" )
{}




defineType(CyclicPairBC);
// addToFactoryTable(BoundaryCondition, CyclicPairBC);
// addToStaticFunctionTable(BoundaryCondition, CyclicPairBC, defaultParameters);

CyclicPairBC::CyclicPairBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const ParameterSet&)
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

void CyclicPairBC::addIntoFieldDictionaries ( OFdicts& dictionaries ) const
{
    for ( const FieldList::value_type& field: OFcase().fields() ) {
        OFDictData::dictFile& fieldDict=dictionaries.addFieldIfNonexistent ( "0/"+field.first, field.second );
        OFDictData::dict& boundaryField=fieldDict.addSubDictIfNonexistent ( "boundaryField" );

        if ( OFversion() >=210 ) {
            OFDictData::dict& BC=boundaryField.addSubDictIfNonexistent ( patchName_+"_half0" );
            OFDictData::dict& BC1=boundaryField.addSubDictIfNonexistent ( patchName_+"_half1" );

            if ( ( ( field.first=="motionU" ) || ( field.first=="pointDisplacement" ) ) ) {
                MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC );
                MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC1 );
            } else {
                BC["type"]="cyclic";
                BC1["type"]="cyclic";
            }
        } else {
            OFDictData::dict& BC=boundaryField.addSubDictIfNonexistent ( patchName_ );

            if ( ( ( field.first=="motionU" ) || ( field.first=="pointDisplacement" ) ) ) {
                MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC );
            } else {
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





GGIBCBase::GGIBCBase(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, 
	const ParameterSet&ps )
: BoundaryCondition(c, patchName, boundaryDict),
  p_(ps)
{
}

void GGIBCBase::modifyMeshOnDisk(const OpenFOAMCase& cm, const boost::filesystem::path& location) const
{
  if (OFversion()<170)
  {
    setSet
    (
      cm, location,
      list_of<std::string>    
      ("faceSet "+p_.zone+" new patchToFace "+patchName_)
    );
    cm.executeCommand(location, "setsToZones", list_of<std::string>("-noFlipMap") );
  }
}



defineType(GGIBC);
addToFactoryTable(BoundaryCondition, GGIBC);
addToStaticFunctionTable(BoundaryCondition, GGIBC, defaultParameters);

GGIBC::GGIBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, 
	const ParameterSet&ps )
: GGIBCBase(c, patchName, boundaryDict, ps),
  p_(ps)
{
}

void GGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  if (OFversion()>=210)
  {
    bndDict["type"]="cyclicAMI";
    bndDict["neighbourPatch"]= p_.shadowPatch;
    bndDict["matchTolerance"]= 0.001;
    bndDict["lowWeightCorrection"]=0.1;
    //bndDict["transform"]= "rotational";    
  }
  else
  {
    bndDict["type"]="ggi";
    bndDict["shadowPatch"]= p_.shadowPatch;
    bndDict["separationOffset"]=OFDictData::vector3(p_.separationOffset);
    bndDict["bridgeOverlap"]=p_.bridgeOverlap;
    bndDict["zone"]=p_.zone;
  }
}

void GGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  for (const FieldList::value_type& field: OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    
    if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      MeshMotionBC::noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
    {
      if (OFversion()>=220)
	BC["type"]=OFDictData::data("cyclicAMI");
      else
	BC["type"]=OFDictData::data("ggi");
    }
  }
}


defineType(CyclicGGIBC);
addToFactoryTable(BoundaryCondition, CyclicGGIBC);
addToStaticFunctionTable(BoundaryCondition, CyclicGGIBC, defaultParameters);

CyclicGGIBC::CyclicGGIBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict,
                  const ParameterSet&ps )
: GGIBCBase(c, patchName, boundaryDict, ps),
  p_(ps)
{
}

void CyclicGGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  if (OFversion()>=210)
  {
    bndDict["type"]="cyclicAMI";
    bndDict["neighbourPatch"]= p_.shadowPatch;
    bndDict["matchTolerance"]= 0.001;
    if (arma::norm(p_.separationOffset,2)<1e-10)
    {
        bndDict["transform"]= "rotational";    
    }
    else
    {
        bndDict["transform"]= "translational";    
    }
    bndDict["rotationCentre"]=OFDictData::vector3(p_.rotationCentre);
    bndDict["rotationAxis"]=OFDictData::vector3(p_.rotationAxis);
    bndDict["separationVector"]=OFDictData::vector3(p_.separationOffset);
    bndDict["rotationAngle"]=p_.rotationAngle;
    bndDict["lowWeightCorrection"]=0.1;
  }
  else
  {
    bndDict["type"]="cyclicGgi";
    bndDict["shadowPatch"]= p_.shadowPatch;
    bndDict["separationOffset"]=OFDictData::vector3(p_.separationOffset);
    bndDict["bridgeOverlap"]=p_.bridgeOverlap;
    bndDict["rotationAxis"]=OFDictData::vector3(p_.rotationAxis);
    bndDict["rotationAngle"]=p_.rotationAngle;
    bndDict["zone"]=p_.zone;
  }
}

void CyclicGGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  for (const FieldList::value_type& field: OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    
    if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      MeshMotionBC::noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
    {
      if (OFversion()>=220)
	BC["type"]=OFDictData::data("cyclicAMI");
      else
	BC["type"]=OFDictData::data("cyclicGgi");
    }
  }
}




defineType(OverlapGGIBC);
addToFactoryTable(BoundaryCondition, OverlapGGIBC);
addToStaticFunctionTable(BoundaryCondition, OverlapGGIBC, defaultParameters);

OverlapGGIBC::OverlapGGIBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, 
	const ParameterSet&ps )
: GGIBCBase(c, patchName, boundaryDict, ps),
  p_(ps)
{
}

void OverlapGGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  
  bndDict["type"]="overlapGgi";
  bndDict["shadowPatch"]= p_.shadowPatch;
  bndDict["bridgeOverlap"]=p_.bridgeOverlap;
  bndDict["separationOffset"]=OFDictData::vector3(p_.separationOffset);
  bndDict["rotationAxis"]=OFDictData::vector3(p_.rotationAxis);
  bndDict["nCopies"]=p_.nCopies;
  bndDict["zone"]=p_.zone;
}

void OverlapGGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  for (const FieldList::value_type& field: OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    
    if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      MeshMotionBC::noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
    {
      BC["type"]=OFDictData::data("overlapGgi");
    }
  }
}




defineType(MixingPlaneGGIBC);
addToFactoryTable(BoundaryCondition, MixingPlaneGGIBC);
addToStaticFunctionTable(BoundaryCondition, MixingPlaneGGIBC, defaultParameters);

MixingPlaneGGIBC::MixingPlaneGGIBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const ParameterSet&ps 
) : GGIBCBase(c, patchName, boundaryDict, ps),
  p_(ps)
{
  if ((OFversion()<160) || (OFversion()>=170))
  {
    throw insight::Exception("MixingPlane interface is not available in the selected OF version!");
  }
}

void MixingPlaneGGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  bndDict["type"]="mixingPlane";
  bndDict["shadowPatch"]= p_.shadowPatch;
  bndDict["separationOffset"]=OFDictData::vector3(p_.separationOffset);
  bndDict["bridgeOverlap"]=p_.bridgeOverlap;
  bndDict["zone"]=p_.zone;
  
  OFDictData::dict csDict;
  csDict["type"]="cylindrical";
  csDict["name"]="mixingCS";
  csDict["origin"]=OFDictData::vector3(vec3(0,0,0));
  csDict["e1"]=OFDictData::vector3(vec3(1,0,0)); //radial
  csDict["e3"]=OFDictData::vector3(vec3(0,0,1)); // rot axis
  csDict["inDegrees"]=true;
  bndDict["coordinateSystem"]=csDict;

  OFDictData::dict rpDict;
  rpDict["sweepAxis"]="Theta";
  rpDict["stackAxis"]="R"; // axial interface
  rpDict["discretisation"]="bothPatches";
  bndDict["ribbonPatch"]=rpDict;

}

void MixingPlaneGGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  for (const FieldList::value_type& field: OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    
    if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      MeshMotionBC::noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
    {
      BC["type"]=OFDictData::data("mixingPlane");
    }
  }
}


void MixingPlaneGGIBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoDictionaries(dictionaries);

  OFDictData::dict& fvSchemes=dictionaries.addDictionaryIfNonexistent("system/fvSchemes");
  
  OFDictData::dict mpd;
  mpd["default"]="areaAveraging";
  mpd["p"]="areaAveraging";
  mpd["U"]="areaAveraging";
  mpd["k"]="fluxAveraging";
  mpd["epsilon"]="fluxAveraging";
  mpd["omega"]="fluxAveraging";
  mpd["nuTilda"]="fluxAveraging";

  if (fvSchemes.find("mixingPlane")==fvSchemes.end())
    fvSchemes["mixingPlane"]=mpd;
  
#warning There is probably a better location for this setup modification
  OFDictData::dict& fvSolution=dictionaries.addDictionaryIfNonexistent("system/fvSolution");
  fvSolution.subDict("solvers")["p"]=stdSymmSolverSetup(1e-7, 0.01);

}





defineType(SuctionInletBC);
addToFactoryTable(BoundaryCondition, SuctionInletBC);
addToStaticFunctionTable(BoundaryCondition, SuctionInletBC, defaultParameters);




SuctionInletBC::SuctionInletBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const ParameterSet& ps
)
: BoundaryCondition(c, patchName, boundaryDict),
  ps_(ps)
{
 BCtype_="patch";
}




void SuctionInletBC::addIntoFieldDictionaries ( OFdicts& dictionaries ) const
{
    Parameters p(ps_);
    multiphaseBC::multiphaseBCPtr phasefractions = 
        multiphaseBC::multiphaseBC::create( ps_.get<SelectableSubsetParameter>("phasefractions") );
    
    BoundaryCondition::addIntoFieldDictionaries ( dictionaries );

        
    phasefractions->addIntoDictionaries ( dictionaries );

    for ( const FieldList::value_type& field: OFcase().fields() ) {
        OFDictData::dict& BC=dictionaries.addFieldIfNonexistent ( "0/"+field.first, field.second )
                             .subDict ( "boundaryField" ).subDict ( patchName_ );
        if ( ( field.first=="U" ) && ( get<0> ( field.second ) ==vectorField ) ) {
            BC["type"]=OFDictData::data ( "pressureInletOutletVelocity" );
            BC["value"]=OFDictData::data ( "uniform ( 0 0 0 )" );
        } else if (
            ( field.first=="T" )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            BC["type"]=OFDictData::data ( "inletOutletTotalTemperature" );
            BC["inletValue"]="uniform "+lexical_cast<string> ( p.T );
            BC["T0"]="uniform "+lexical_cast<string> ( p.T );
            BC["U"]=OFDictData::data ( p.UName );
            BC["phi"]=OFDictData::data ( p.phiName );
            BC["psi"]=OFDictData::data ( p.psiName );
            BC["gamma"]=OFDictData::data ( p.gamma );
            BC["value"]="uniform "+lexical_cast<string> ( p.T );
        } else if (
            ( ( field.first=="p" ) || ( field.first=="pd" ) || ( field.first=="p_rgh" ) )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            BC["type"]=OFDictData::data ( "totalPressure" );
            BC["p0"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( p.pressure ) );
            BC["U"]=OFDictData::data ( p.UName );
            BC["phi"]=OFDictData::data ( p.phiName );
            BC["rho"]=OFDictData::data ( p.rhoName );
            BC["psi"]=OFDictData::data ( p.psiName );
            BC["gamma"]=OFDictData::data ( p.gamma );
            BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( p.pressure ) );
        }
        else if ( ( field.first=="rho" ) && ( get<0> ( field.second ) ==scalarField ) )
          {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( p.rho ) );
          }
        else if ( ( field.first=="k" ) && ( get<0> ( field.second ) ==scalarField ) )
          {
            BC["type"]=OFDictData::data ( "turbulentIntensityKineticEnergyInlet" );
            BC["intensity"]=p.turb_I;
            BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( 0.1 ) );
          }
        else if ( ( field.first=="omega" ) && ( get<0> ( field.second ) ==scalarField ) )
          {
            BC["type"]=OFDictData::data ( "turbulentMixingLengthFrequencyInlet" );
            BC["mixingLength"]=p.turb_L;
            BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( 1.0 ) );
          }
        else if ( ( field.first=="epsilon" ) && ( get<0> ( field.second ) ==scalarField ) )
          {
            BC["type"]=OFDictData::data ( "turbulentMixingLengthDissipationRateInlet" );
            BC["mixingLength"]=p.turb_L;
            BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( 1.0 ) );
          }
        else if ( (
                    ( field.first=="nut" ) ||
                    ( field.first=="nuSgs" )
                   ) && ( get<0> ( field.second ) ==scalarField ) )
          {
            BC["type"]=OFDictData::data ( "calculated" );
            BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( 0.0 ) );
          }
        else if
          (
              (
                ( field.first=="nuTilda" )
              )
              &&
              ( get<0> ( field.second ) ==scalarField )
          )
          {
            BC["type"]=OFDictData::data ( "zeroGradient" );
          }
        else
          {
            if ( ! (
                        MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC )
                        ||
                        phasefractions->addIntoFieldDictionary ( field.first, field.second, BC )
                    ) )
                //throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
            {
                BC["type"]=OFDictData::data ( "zeroGradient" );
            }
          }
    }
}




defineType(MassflowBC);
addToFactoryTable(BoundaryCondition, MassflowBC);
addToStaticFunctionTable(BoundaryCondition, MassflowBC, defaultParameters);


MassflowBC::MassflowBC
(
    OpenFOAMCase& c,
    const std::string& patchName,
    const OFDictData::dict& boundaryDict,
    const ParameterSet& ps
)
    : BoundaryCondition ( c, patchName, boundaryDict ),
      ps_ ( ps )
{
    BCtype_="patch";
}


void MassflowBC::addIntoFieldDictionaries ( OFdicts& dictionaries ) const
{
    Parameters p ( ps_ );
    
    turbulenceBC::turbulenceBCPtr turbulence =
        turbulenceBC::turbulenceBC::create ( ps_.get<SelectableSubsetParameter> ( "turbulence" ) );
    multiphaseBC::multiphaseBCPtr phasefractions =
        multiphaseBC::multiphaseBC::create ( ps_.get<SelectableSubsetParameter> ( "phasefractions" ) );

    BoundaryCondition::addIntoFieldDictionaries ( dictionaries );
    phasefractions->addIntoDictionaries ( dictionaries );

    double velocity=1.0; // required for turbulence quantities. No better idea yet...
    for ( const FieldList::value_type& field: OFcase().fields() ) {
        OFDictData::dict& BC=dictionaries.addFieldIfNonexistent ( "0/"+field.first, field.second )
                             .subDict ( "boundaryField" ).subDict ( patchName_ );
        if ( ( field.first=="U" ) && ( get<0> ( field.second ) ==vectorField ) ) {
            BC["type"]=OFDictData::data ( "flowRateInletVelocity" );
            if (const auto *mf = boost::get<Parameters::flowrate_massflow_type>(&p.flowrate))
            {
                BC["rho"]=p.rhoName;
                BC["massFlowRate"]=mf->value;
            }
            else if (const auto *vf = boost::get<Parameters::flowrate_volumetric_type>(&p.flowrate))
            {
                BC["volumetricFlowRate"]=vf->value;
            }
            BC["value"]=OFDictData::data ( "uniform ( 0 0 0 )" );
        } else if (
            ( field.first=="T" )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]="uniform "+lexical_cast<string> ( p.T );
//            BC["type"]=OFDictData::data ( "inletOutletTotalTemperature" );
//            BC["inletValue"]="uniform "+lexical_cast<string> ( p.T );
//            BC["T0"]="uniform "+lexical_cast<string> ( p.T );
//            BC["U"]=OFDictData::data ( p.UName );
//            BC["phi"]=OFDictData::data ( p.phiName );
//            BC["psi"]=OFDictData::data ( p.psiName );
//            BC["gamma"]=OFDictData::data ( p.gamma );
//            BC["value"]="uniform "+lexical_cast<string> ( p.T );
        } else if (
            ( ( field.first=="pd" ) || ( field.first=="p_rgh" ) )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            if ( OFversion() >=210 ) {
                BC["type"]=OFDictData::data ( "fixedFluxPressure" );
            } else {
                BC["type"]=OFDictData::data ( "buoyantPressure" );
            }
//       BC["type"]=OFDictData::data("calculated");
//       BC["value"]=OFDictData::data("uniform 0");
        } else if ( ( field.first=="rho" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( p.rho ) );
        } else if ( ( field.first=="k" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_k ( BC, velocity );
        } else if ( ( field.first=="omega" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_omega ( BC, velocity );
        } else if ( ( field.first=="epsilon" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_epsilon ( BC, velocity );
        } else if ( ( field.first=="nut" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "calculated" );
            BC["value"]="uniform "+lexical_cast<string> ( 1e-10 );
        } else if ( ( field.first=="nuTilda" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_nuTilda ( BC, velocity );
        } else if ( ( field.first=="R" ) && ( get<0> ( field.second ) ==symmTensorField ) ) {
            turbulence->setDirichletBC_R ( BC, velocity );
        } else if ( ( field.first=="nuSgs" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]="uniform 1e-10";
        } else {
            if ( ! (
                        MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC )
                        ||
                        phasefractions->addIntoFieldDictionary ( field.first, field.second, BC )
                    ) )
                //throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
            {
                BC["type"]=OFDictData::data ( "zeroGradient" );
            }
        }
    }
}



defineType(MappedVelocityInletBC);
addToFactoryTable(BoundaryCondition, MappedVelocityInletBC);
addToStaticFunctionTable(BoundaryCondition, MappedVelocityInletBC, defaultParameters);




MappedVelocityInletBC::MappedVelocityInletBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const ParameterSet& ps
)
: BoundaryCondition(c, patchName, boundaryDict),
  ps_(ps)
{
 BCtype_="patch";
}


void MappedVelocityInletBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  Parameters p(ps_);
  
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
//   if (OFversion()>=210)
//   {
        bndDict["type"]="mappedPatch";
        bndDict["inGroups"]="1(mappedPatch)";
        bndDict["sampleMode"]="nearestCell";
        bndDict["sampleRegion"]="region0";
        bndDict["samplePatch"]="none";
        bndDict["offsetMode"]="uniform";
        bndDict["offset"]=OFDictData::vector3(p.distance);
    //bndDict["transform"]= "rotational";    
//   }
//   else
//   {
//   }
}

void MappedVelocityInletBC::addIntoFieldDictionaries ( OFdicts& dictionaries ) const
{
    Parameters p(ps_);
    multiphaseBC::multiphaseBCPtr phasefractions = 
        multiphaseBC::multiphaseBC::create( ps_.get<SelectableSubsetParameter>("phasefractions") );
    
    BoundaryCondition::addIntoFieldDictionaries ( dictionaries );

        
    phasefractions->addIntoDictionaries ( dictionaries );

    for ( const FieldList::value_type& field: OFcase().fields() ) {
        OFDictData::dict& BC=dictionaries.addFieldIfNonexistent ( "0/"+field.first, field.second )
                             .subDict ( "boundaryField" ).subDict ( patchName_ );
        if ( ( field.first=="U" ) && ( get<0> ( field.second ) ==vectorField ) ) {
            BC["type"]="mapped";
            BC["fieldName"]="U";
            BC["setAverage"]=true;
            BC["interpolationScheme"]="cell";
            BC["average"]=OFDictData::vector3(p.average);
            BC["value"]=OFDictData::data ( "uniform ( 0 0 0 )" );
        } else if (
            ( field.first=="T" )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]="uniform "+lexical_cast<string> ( p.T );
        } else if (
            ( ( field.first=="p" ) || ( field.first=="pd" ) || ( field.first=="p_rgh" ) )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            BC["type"]=OFDictData::data ( "zeroGradient" );
        } else if ( ( field.first=="rho" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( p.rho ) );
        } else if
        (
            (
                ( field.first=="k" ) ||
                ( field.first=="epsilon" ) ||
                ( field.first=="omega" ) ||
                ( field.first=="nut" ) ||
                ( field.first=="nuSgs" ) ||
                ( field.first=="nuTilda" )
            )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            BC["type"]=OFDictData::data ( "zeroGradient" );
        } else {
            if ( ! (
                        MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC )
                        ||
                        phasefractions->addIntoFieldDictionary ( field.first, field.second, BC )
                    ) )
                //throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
            {
                BC["type"]=OFDictData::data ( "zeroGradient" );
            }
        }
    }
}




defineType(VelocityInletBC);
addToFactoryTable(BoundaryCondition, VelocityInletBC);
addToStaticFunctionTable(BoundaryCondition, VelocityInletBC, defaultParameters);

VelocityInletBC::VelocityInletBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const ParameterSet& ps,
  const boost::filesystem::path& casedir
)
: BoundaryCondition(c, patchName, boundaryDict),
  ps_(ps),
  casedir_(casedir)
{
 BCtype_="patch";
}


void VelocityInletBC::setField_p(OFDictData::dict& BC) const
{
  BC["type"]=OFDictData::data("zeroGradient");
}

void VelocityInletBC::setField_U(OFDictData::dict& BC) const
{
    Parameters p ( ps_ );
    FieldData(p.velocity, casedir_).setDirichletBC(BC);
//   FieldData(ps_.get<SelectableSubsetParameter>("velocity")()).setDirichletBC(BC);
}

void VelocityInletBC::addIntoFieldDictionaries ( OFdicts& dictionaries) const
{
    Parameters p ( ps_ );
    
    turbulenceBC::turbulenceBCPtr turbulence =
        turbulenceBC::turbulenceBC::create ( ps_.get<SelectableSubsetParameter> ( "turbulence" ) );
    multiphaseBC::multiphaseBCPtr phasefractions =
        multiphaseBC::multiphaseBC::create ( ps_.get<SelectableSubsetParameter> ( "phasefractions" ) );
        
    FieldData velocity(ps_.getSubset("velocity"), casedir_), T(ps_.getSubset("T"), casedir_), rho(ps_.getSubset("rho"), casedir_);

    BoundaryCondition::addIntoFieldDictionaries ( dictionaries );
    phasefractions->addIntoDictionaries ( dictionaries );

    for ( const FieldList::value_type& field: OFcase().fields() ) {
        OFDictData::dict& BC=dictionaries.addFieldIfNonexistent ( "0/"+field.first, field.second )
                             .subDict ( "boundaryField" ).subDict ( patchName_ );
        if ( ( field.first=="U" ) && ( get<0> ( field.second ) ==vectorField ) ) {
            setField_U ( BC );
        }

        else if (
            ( field.first=="p" ) && ( get<0> ( field.second ) ==scalarField )
        ) {
            setField_p ( BC );
        } else if (
            ( field.first=="T" )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            T.setDirichletBC ( BC );
//       BC["type"]=OFDictData::data("fixedValue");
//       BC["value"]="uniform "+lexical_cast<string>(p_.T());
        } else if (
            ( ( field.first=="pd" ) || ( field.first=="p_rgh" ) )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            if ( OFversion() >=210 ) {
                BC["type"]=OFDictData::data ( "fixedFluxPressure" );
            } else {
                BC["type"]=OFDictData::data ( "buoyantPressure" );
            }
//       BC["type"]=OFDictData::data("calculated");
//       BC["value"]=OFDictData::data("uniform 0");
        }

        else if ( ( field.first=="rho" ) && ( get<0> ( field.second ) ==scalarField ) ) {
//       BC["type"]=OFDictData::data("fixedValue");
//       BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
            rho.setDirichletBC ( BC );
        } else if ( ( field.first=="k" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_k ( BC, velocity.representativeValueMag() );
        } else if ( ( field.first=="omega" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_omega ( BC, velocity.representativeValueMag() );
        } else if ( ( field.first=="epsilon" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_epsilon ( BC, velocity.representativeValueMag() );
        } else if ( ( field.first=="nut" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "calculated" );
            BC["value"]="uniform "+lexical_cast<string> ( 1e-10 );
        } else if ( ( field.first=="nuTilda" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_nuTilda ( BC, velocity.representativeValueMag() );
        } else if ( ( field.first=="R" ) && ( get<0> ( field.second ) ==symmTensorField ) ) {
            turbulence->setDirichletBC_R ( BC, velocity.representativeValueMag() );
        } else if ( ( field.first=="nuSgs" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]="uniform 1e-10";
        } else {
            if ( ! (
                        MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC )
                        ||
                        phasefractions->addIntoFieldDictionary ( field.first, field.second, BC )
                    ) ) {
                BC["type"]=OFDictData::data ( "zeroGradient" );
            }
            //throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
        }
    }
}




defineType(ExptDataInletBC);
addToFactoryTable(BoundaryCondition, ExptDataInletBC);
addToStaticFunctionTable(BoundaryCondition, ExptDataInletBC, defaultParameters);


ExptDataInletBC::ExptDataInletBC
(
  OpenFOAMCase& c, 
  const string& patchName, 
  const OFDictData::dict& boundaryDict,
  const ParameterSet& ps
)
: BoundaryCondition(c, patchName, boundaryDict),
  ps_(ps)
{
 BCtype_="patch";
}

void ExptDataInletBC::addDataDict(OFdicts& dictionaries, const std::string& prefix, const std::string& fieldname, const arma::mat& data) const
{
  OFDictData::dictFile& Udict=dictionaries.addDictionaryIfNonexistent(prefix+"/0/"+fieldname);
  Udict.isSequential=true;
  
  if (data.n_cols==1)
    Udict["a"]=0.0;
  else if (data.n_cols==3)
    Udict["a"]=OFDictData::vector3(vec3(0,0,0));
  else
    throw insight::Exception("Unhandled number of components: "+lexical_cast<std::string>(data.n_cols));
  
  OFDictData::list vals;
  for (size_t r=0; r<data.n_rows; r++)
  {
    if (data.n_cols==1)
      vals.push_back(data(r));
    else if (data.n_cols==3)
      vals.push_back(OFDictData::vector3(data.row(r).t()));
  }
  Udict["b"]=vals;
}

void ExptDataInletBC::addIntoFieldDictionaries ( OFdicts& dictionaries ) const
{
    Parameters p ( ps_ );
    multiphaseBC::multiphaseBCPtr phasefractions =
        multiphaseBC::multiphaseBC::create ( ps_.get<SelectableSubsetParameter> ( "phasefractions" ) );

    BoundaryCondition::addIntoFieldDictionaries ( dictionaries );
    phasefractions->addIntoDictionaries ( dictionaries );

    std::string prefix="constant/boundaryData/"+patchName_;

    size_t np=p.data.size();
    arma::mat ptdat = arma::zeros ( np, 3 );
    arma::mat velocity = arma::zeros ( np, 3 );
    arma::mat TKE = arma::zeros ( np );
    arma::mat epsilon = arma::zeros ( np );
    size_t j=0;
    for ( const Parameters::data_default_type& pt: p.data ) {
        ptdat.row ( j ) =pt.point;
        velocity.row ( j ) =pt.velocity;
        TKE ( j ) =pt.k;
        epsilon ( j ) =pt.epsilon;
        j++;
    }

    OFDictData::dictFile& ptsdict=dictionaries.addDictionaryIfNonexistent ( prefix+"/points" );
    ptsdict.isSequential=true;
    OFDictData::list pts;
    for ( size_t r=0; r<ptdat.n_rows; r++ ) {
        pts.push_back ( OFDictData::vector3 ( ptdat.row ( r ).t() ) );
    }
    ptsdict["a"]=pts;


    for ( const FieldList::value_type& field: OFcase().fields() ) {
        OFDictData::dict& BC=dictionaries.addFieldIfNonexistent ( "0/"+field.first, field.second )
                             .subDict ( "boundaryField" ).subDict ( patchName_ );

        if ( ( field.first=="U" ) && ( get<0> ( field.second ) ==vectorField ) ) {
            BC["type"]=OFDictData::data ( "timeVaryingMappedFixedValue" );
            BC["offset"]=OFDictData::vector3 ( vec3 ( 0,0,0 ) );
            BC["setAverage"]=false;
            BC["perturb"]=1e-3;

//       OFDictData::dictFile& Udict=dictionaries.addDictionaryIfNonexistent(prefix+"/0/U");
//       Udict.isSequential=true;
//       Udict["a"]=OFDictData::vector3(vec3(0,0,0));
//
//       OFDictData::list vals;
//       const arma::mat& Udat=p_.velocity();
//       cout<<Udat<<endl;
//       for (int r=0; r<Udat.n_rows; r++)
// 	vals.push_back(OFDictData::vector3(Udat.row(r).t()));
//       Udict["b"]=vals;
            addDataDict ( dictionaries, prefix, "U", velocity );
        }

        else if (
            ( field.first=="p" ) && ( get<0> ( field.second ) ==scalarField )
        ) {
            BC["type"]=OFDictData::data ( "zeroGradient" );
        }
//     else if (
//       (field.first=="T")
//       &&
//       (get<0>(field.second)==scalarField)
//     )
//     {
//       BC["type"]=OFDictData::data("fixedValue");
//       BC["value"]="uniform "+lexical_cast<string>(p_.T());
//     }
        else if (
            ( ( field.first=="pd" ) || ( field.first=="p_rgh" ) )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            if ( OFversion() >=210 ) {
                BC["type"]=OFDictData::data ( "fixedFluxPressure" );
            } else {
                BC["type"]=OFDictData::data ( "buoyantPressure" );
            }
//       BC["type"]=OFDictData::data("calculated");
//       BC["value"]=OFDictData::data("uniform 0");
        }

//     else if ( (field.first=="rho") && (get<0>(field.second)==scalarField) )
//     {
//       BC["type"]=OFDictData::data("fixedValue");
//       BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
//     }
        else if ( ( field.first=="k" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "timeVaryingMappedFixedValue" );
            BC["offset"]=0.0;
            BC["setAverage"]=false;
            BC["perturb"]=1e-3;
            addDataDict ( dictionaries, prefix, "k", TKE );
        } else if ( ( field.first=="omega" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "timeVaryingMappedFixedValue" );
            BC["offset"]=0.0;
            BC["setAverage"]=false;
            BC["perturb"]=1e-3;
            addDataDict ( dictionaries, prefix, "omega", epsilon/ ( 0.09*TKE ) );
        } else if ( ( field.first=="epsilon" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "timeVaryingMappedFixedValue" );
            BC["offset"]=0.0;
            BC["setAverage"]=false;
            BC["perturb"]=1e-3;
            addDataDict ( dictionaries, prefix, "epsilon", epsilon );
        } else if ( ( field.first=="nut" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            double nutilda=1e-10; //sqrt(1.5)*p_.turbulenceIntensity() * arma::norm(p_.velocity(), 2) * p_.mixingLength();
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]="uniform "+lexical_cast<string> ( nutilda );
        }
//     else if ( (field.first=="nuTilda") && (get<0>(field.second)==scalarField) )
//     {
//       BC["type"]=OFDictData::data("timeVaryingMappedFixedValue");
//       BC["offset"]=0.0;
//       BC["setAverage"]=false;
// //       addDataDict(dictionaries, prefix, "nuTilda", p_.epsilon());
//     }
        else if ( ( field.first=="nuSgs" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]="uniform 1e-10";
        } else {
            if ( ! (
                        MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC )
                        ||
                        phasefractions->addIntoFieldDictionary ( field.first, field.second, BC )
                    ) ) {
                BC["type"]=OFDictData::data ( "zeroGradient" );
            }
            //throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
        }
    }
}





defineType(CompressibleInletBC);
addToFactoryTable(BoundaryCondition, CompressibleInletBC);
addToStaticFunctionTable(BoundaryCondition, CompressibleInletBC, defaultParameters);


CompressibleInletBC::CompressibleInletBC
(
    OpenFOAMCase& c,
    const string& patchName,
    const OFDictData::dict& boundaryDict,
    const ParameterSet& ps
)
    : VelocityInletBC ( c, patchName, boundaryDict, ps ),
      ps_ ( ps )
{
    BCtype_="patch";
}

void CompressibleInletBC::setField_p ( OFDictData::dict& BC ) const
{
    BC["type"]=OFDictData::data ( "fixedValue" );
    BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( Parameters ( ps_ ).pressure ) );
}





defineType(TurbulentVelocityInletBC);
addToFactoryTable(BoundaryCondition, TurbulentVelocityInletBC);
addToStaticFunctionTable(BoundaryCondition, TurbulentVelocityInletBC, defaultParameters);

TurbulentVelocityInletBC::TurbulentVelocityInletBC
(
  OpenFOAMCase& c,
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const ParameterSet& ps
)
: BoundaryCondition(c, patchName, boundaryDict),
  ps_(ps),
  p_(ps)
{
 BCtype_="patch";
}

const std::vector<std::string> TurbulentVelocityInletBC::inflowGenerator_types = boost::assign::list_of
   ("inflowGenerator<hatSpot>")
   ("inflowGenerator<gaussianSpot>")
   ("inflowGenerator<decayingTurbulenceSpot>")
   ("inflowGenerator<decayingTurbulenceVorton>")
   ("inflowGenerator<anisotropicVorton_Analytic>")
   ("inflowGenerator<anisotropicVorton_PseudoInv>")
   ("inflowGenerator<anisotropicVorton_NumOpt>")
   ("inflowGenerator<anisotropicVorton2>")
   ("inflowGenerator<combinedVorton>")
   ("modalTurbulence")
   .convert_to_container<std::vector<std::string> >();
   
void TurbulentVelocityInletBC::setField_U(OFDictData::dict& BC) const
{
  ParameterSet ps=Parameters::makeDefault();
  p_.set(ps);
  
  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu 
	= boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(&p_.turbulence))
  {
    FieldData(ps.getSubset("umean"), ".").setDirichletBC(BC);
  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu 
	= boost::get<Parameters::turbulence_inflowGenerator_type>(&p_.turbulence))
  {
    BC["type"]= inflowGenerator_types[tu->type];
    BC["Umean"]=FieldData(ps.getSubset("umean"), ".").sourceEntry();
    BC["c"]=FieldData(tu->volexcess).sourceEntry();
    BC["uniformConvection"]=tu->uniformConvection;
    BC["R"]=FieldData(ps.get<SelectableSubsetParameter>("turbulence")().getSubset("R"), ".").sourceEntry();
    BC["L"]=FieldData(ps.get<SelectableSubsetParameter>("turbulence")().getSubset("L"), ".").sourceEntry();
    BC["value"]="uniform (0 0 0)";
  }
}

void TurbulentVelocityInletBC::setField_p(OFDictData::dict& BC) const
{
  BC["type"]=OFDictData::data("zeroGradient");
}

void TurbulentVelocityInletBC::setField_k(OFDictData::dict& BC) const
{
  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu 
	= boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(&p_.turbulence))
  {

    double U=FieldData( ParameterSet(p_).getSubset("umean"), "." ).representativeValueMag();
    
    double uprime=tu->intensity*U;
    double k=max(1e-6, 3.*pow(uprime, 2)/2.);
    BC["type"]="fixedValue";
    BC["value"]="uniform "+lexical_cast<string>(k);
    
  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu 
	= boost::get<Parameters::turbulence_inflowGenerator_type>(&p_.turbulence))
  {
    // set some small sgs energy
    BC["type"]="fixedValue";
    BC["value"]="uniform 1e-5";
  }
}

void TurbulentVelocityInletBC::setField_omega(OFDictData::dict& BC) const
{
  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu 
	= boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(&p_.turbulence))
  {

    double U=FieldData( ParameterSet(p_).getSubset("umean"), "." ).representativeValueMag();
    
    double uprime = tu->intensity*U;
    double k = max(1e-6, 3.*pow(uprime, 2)/2.);
    double omega = sqrt(k) / tu->lengthScale;
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]="uniform "+lexical_cast<string>(omega);
    
  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu 
	= boost::get<Parameters::turbulence_inflowGenerator_type>(&p_.turbulence))
  {
    throw insight::Exception("Requested BC for field omega while inflow generator was selected!");
  }
}

void TurbulentVelocityInletBC::setField_epsilon(OFDictData::dict& BC) const
{

  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu 
	= boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(&p_.turbulence))
  {

    double U=FieldData( ParameterSet(p_).getSubset("umean"), "." ).representativeValueMag();
    
    double uprime = tu->intensity*U;
    double k=3.*pow(uprime, 2)/2.;
    double epsilon=0.09*pow(k, 1.5)/tu->lengthScale;
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]="uniform "+lexical_cast<string>(epsilon);
    
  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu 
	= boost::get<Parameters::turbulence_inflowGenerator_type>(&p_.turbulence))
  {
    throw insight::Exception("Requested BC for field epsilon while inflow generator was selected!");
  }

}

void TurbulentVelocityInletBC::setField_nuTilda(OFDictData::dict& BC) const
{
  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu 
	= boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(&p_.turbulence))
  {

    double U=FieldData( ParameterSet(p_).getSubset("umean"), "." ).representativeValueMag();
    
    double uprime = tu->intensity*U;
    double nutilda=sqrt(1.5)* uprime * tu->lengthScale;
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]="uniform "+lexical_cast<string>(nutilda);
    
  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu 
	= boost::get<Parameters::turbulence_inflowGenerator_type>(&p_.turbulence))
  {
    throw insight::Exception("Requested BC for field nuTilda while inflow generator was selected!");
  }

}

void TurbulentVelocityInletBC::setField_R(OFDictData::dict& BC) const
{
  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu 
	= boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(&p_.turbulence))
  {

    double U=FieldData( ParameterSet(p_).getSubset("umean"), "." ).representativeValueMag();
    
    double uprime=tu->intensity*U;
    double kBy3=max(1e-6, pow(uprime, 2)/2.);
    BC["type"]="fixedValue";
    BC["value"]="uniform ("+str(format("%g 0 0 %g 0 %g") % kBy3 % kBy3 % kBy3 )+")";
    
  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu 
	= boost::get<Parameters::turbulence_inflowGenerator_type>(&p_.turbulence))
  {
    throw insight::Exception("Requested BC for field R while inflow generator was selected!");
  }
}


void TurbulentVelocityInletBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  multiphaseBC::multiphaseBCPtr phasefractions =
        multiphaseBC::multiphaseBC::create ( ps_.get<SelectableSubsetParameter> ( "phasefractions" ) );
    
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  
  if (boost::get<Parameters::turbulence_inflowGenerator_type>(&p_.turbulence))  
    controlDict.addListIfNonexistent("libs").push_back( OFDictData::data("\"libinflowGeneratorBC.so\"") );

  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  phasefractions->addIntoDictionaries ( dictionaries );  
//   p_.phasefractions()->addIntoDictionaries(dictionaries);
  
  for (const FieldList::value_type& field: OFcase().fields())
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
      setField_p(BC);
    }
//     else if ( 
//       (field.first=="T") 
//       && 
//       (get<0>(field.second)==scalarField) 
//     )
//     {
//       p_.T().setDirichletBC(BC);
// //       BC["type"]=OFDictData::data("fixedValue");
// //       BC["value"]="uniform "+lexical_cast<string>(p_.T());
//     }    
//     else if ( 
//       ( (field.first=="pd") || (field.first=="p_rgh") )
//       && 
//       (get<0>(field.second)==scalarField) 
//     )
//     {
//       if (OFversion()>=210)
// 	BC["type"]=OFDictData::data("fixedFluxPressure");
//       else
// 	BC["type"]=OFDictData::data("buoyantPressure");
// //       BC["type"]=OFDictData::data("calculated");
// //       BC["value"]=OFDictData::data("uniform 0");
//     }
    
//     else if ( (field.first=="rho") && (get<0>(field.second)==scalarField) )
//     {
// //       BC["type"]=OFDictData::data("fixedValue");
// //       BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
//       p_.rho().setDirichletBC(BC);
//     }
    else if ( (field.first=="k") && (get<0>(field.second)==scalarField) )
    {
      setField_k(BC);
    }
    else if ( (field.first=="omega") && (get<0>(field.second)==scalarField) )
    {
      setField_omega(BC);
    }
    else if ( (field.first=="epsilon") && (get<0>(field.second)==scalarField) )
    {
      setField_epsilon(BC);
    }
    else if ( (field.first=="nut") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("calculated");
      BC["value"]="uniform "+lexical_cast<string>(1e-10);
    }
    else if ( (field.first=="nuTilda") && (get<0>(field.second)==scalarField) )
    {
      setField_nuTilda(BC);
    }
    else if ( (field.first=="R") && (get<0>(field.second)==symmTensorField) )
    {
      setField_R(BC);
    }
    else if ( (field.first=="nuSgs") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]="zeroGradient";
//       BC["value"]="uniform 1e-10";
    }
    else
    {
      if (!(
	  MeshMotionBC::noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
	  ||
	  phasefractions->addIntoFieldDictionary(field.first, field.second, BC)
	  ))
	{
	  BC["type"]=OFDictData::data("zeroGradient");
	}
	//throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
    }
  }
}



// ParameterSet TurbulentVelocityInletBC::defaultParameters()
// {
//   return ParameterSet
//   (
//     boost::assign::list_of<ParameterSet::SingleEntry>
//     ("uniformConvection", new BoolParameter(false, "Whether to use a uniform convection velocity instead of the local mean velocity"))
//     ("volexcess", new DoubleParameter(16.0, "Volumetric overlapping of spots"))
//     (
//       "type", new SelectionParameter(0,
// 	list_of<string>
// 	("inflowGenerator<hatSpot>")
// 	("inflowGenerator<gaussianSpot>")
// 	("inflowGenerator<decayingTurbulenceSpot>")
// 	("inflowGenerator<decayingTurbulenceVorton>")
// 	("inflowGenerator<anisotropicVorton>")
// 	("modalTurbulence")
//       ,
//       "Type of inflow generator")
//     )
//     ("L", FieldData::defaultParameter(vec3(1,1,1), "Origin of the prescribed integral length scale field"))
//     ("R", FieldData::defaultParameter(arma::zeros(6), "Origin of the prescribed reynolds stress field"))
//     .convert_to_container<ParameterSet::EntryList>()
//   );
// }


// void TurbulentVelocityInletBC::initInflowBC(const boost::filesystem::path& location, const ParameterSet& iniparams) const
// {
//   if (p_.initializer())
//   {
//     OFDictData::dictFile inflowProperties;
//     
//     OFDictData::list& initializers = inflowProperties.addListIfNonexistent("initializers");
//     
//     initializers.push_back( p_.initializer()->type() );
//     OFDictData::dict d;
//     p_.initializer()->addToInitializerList(d, patchName_, p_.velocity().representativeValue(), iniparams);
//     initializers.push_back(d);
//     
//     // then write to file
//     inflowProperties.write( location / "constant" / "inflowProperties" );
//     
//     OFcase().executeCommand(location, "initInflowGenerator");
//   }
// }
  
defineType(PressureOutletBC);
addToFactoryTable(BoundaryCondition, PressureOutletBC);
addToStaticFunctionTable(BoundaryCondition, PressureOutletBC, defaultParameters);

PressureOutletBC::PressureOutletBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const ParameterSet& ps,
  const boost::filesystem::path& casedir
)
: BoundaryCondition(c, patchName, boundaryDict),
  ps_(ps),
  casedir_(casedir)
{
 BCtype_="patch";
}

void PressureOutletBC::addIntoFieldDictionaries ( OFdicts& dictionaries ) const
{
    Parameters p ( ps_ );
    multiphaseBC::multiphaseBCPtr phasefractions =
        multiphaseBC::multiphaseBC::create ( ps_.get<SelectableSubsetParameter> ( "phasefractions" ) );

    BoundaryCondition::addIntoFieldDictionaries ( dictionaries );
    phasefractions->addIntoDictionaries(dictionaries);

    if ( (boost::get<Parameters::behaviour_fixMeanValue_type>(&p.behaviour)) && ( OFversion() !=160 ) ) {
        OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent ( "system/controlDict" );
        controlDict.getList ( "libs" ).push_back ( OFDictData::data ( "\"libfixedMeanValueBC.so\"" ) );
    }

    for ( const FieldList::value_type& field: OFcase().fields() ) {
        OFDictData::dict& BC=dictionaries.addFieldIfNonexistent ( "0/"+field.first, field.second )
                             .subDict ( "boundaryField" ).subDict ( patchName_ );

        if ( ( field.first=="U" ) && ( get<0> ( field.second ) ==vectorField ) ) {
            if ( p.prohibitInflow ) {
                BC["type"]=OFDictData::data ( "inletOutlet" );
                BC["inletValue"]=OFDictData::data ( "uniform ( 0 0 0 )" );
                BC["value"]=OFDictData::data ( "uniform ( 0 0 0 )" );
            } else {
                BC["type"]=OFDictData::data ( "zeroGradient" );
                BC["value"]=OFDictData::data ( "uniform ( 0 0 0 )" );
            }
        } else if (
            ( field.first=="T" )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            BC["type"]="zeroGradient";
        } else if (
            ( ( field.first=="p" ) || ( field.first=="pd" ) || ( field.first=="p_rgh" ) )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            if ( (field.first=="p") && (OFcase().hasField("pd")||OFcase().hasField("p_rgh")))
              {
                BC["type"]=OFDictData::data ( "calculated" );
                BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( /*p.pressure*/ 1e5 ) );
              }
            else
              {
                if ( const auto* unif = boost::get<Parameters::behaviour_uniform_type>(&p.behaviour) )
                {
                    FieldData(unif->pressure, casedir_).setDirichletBC(BC);
//                    BC["type"]=OFDictData::data ( "fixedValue" );
//                    BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( unif->pressure ) );
                }
                else if ( const auto* fixmean = boost::get<Parameters::behaviour_fixMeanValue_type>(&p.behaviour) )
                {
                    BC["type"]=OFDictData::data ( "fixedMeanValue" );
                    BC["meanValue"]=OFDictData::data ( fixmean->pressure );
                    BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( fixmean->pressure ) );
                }
                else if ( const auto* wt = boost::get<Parameters::behaviour_waveTransmissive_type>(&p.behaviour) )
                {
                    BC["type"]="waveTransmissive";
                    BC["psi"]="thermo:psi";
                    BC["gamma"]=wt->kappa;
                    BC["lInf"]=wt->L;
                    BC["fieldInf"]=OFDictData::data ( wt->pressure );
                    BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( wt->pressure ) );
                }
                else if ( const auto* wt = boost::get<Parameters::behaviour_removePRGHHydrostaticPressure_type>(&p.behaviour) )
                {
                    BC["type"]=OFDictData::data ( "prghTotalPressure" );
                    BC["p0"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( wt->pressure ) );
                }
              }

        } else if ( ( field.first=="rho" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( p.rho ) );
        } else if
        (
            (
                ( field.first=="k" ) ||
                ( field.first=="epsilon" ) ||
                ( field.first=="omega" ) ||
                ( field.first=="nut" ) ||
                ( field.first=="nuSgs" ) ||
                ( field.first=="nuTilda" )
            )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            BC["type"]=OFDictData::data ( "zeroGradient" );
        } else {
            if ( ! (
                        MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC )
                         ||
                        phasefractions->addIntoFieldDictionary(field.first, field.second, BC)
                    ) )
                //throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
            {
                BC["type"]=OFDictData::data ( "zeroGradient" );
            }

        }
    }
}

PotentialFreeSurfaceBC::PotentialFreeSurfaceBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict
)
: BoundaryCondition(c, patchName, boundaryDict)
{
 BCtype_="patch";
}

void PotentialFreeSurfaceBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  //p_.phasefractions()->addIntoDictionaries(dictionaries);

  for (const FieldList::value_type& field: OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
      
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      BC["type"]=OFDictData::data("pressureInletOutletParSlipVelocity");
      BC["value"]=OFDictData::data("uniform ( 0 0 0 )");
    }
    else if ( 
      (field.first=="T") 
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]="zeroGradient";
    }
    else if ( 
      ( (field.first=="p_gh") )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
	BC["type"]=OFDictData::data("waveSurfacePressure");
	BC["value"]=OFDictData::data("uniform 0");
    }
    else if ( 
      ( (field.first=="p") )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
	BC["type"]=OFDictData::data("zeroGradient");
	//BC["value"]=OFDictData::data("uniform 0");
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
	  MeshMotionBC::noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
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







defineType(WallBC);
addToFactoryTable(BoundaryCondition, WallBC);
addToStaticFunctionTable(BoundaryCondition, WallBC, defaultParameters);


WallBC::WallBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const ParameterSet& ps)
: BoundaryCondition(c, patchName, boundaryDict),
  ps_(ps)
{
  BCtype_="wall";
}

void WallBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  MeshMotionBC::MeshMotionBC::create(ps_.get<SelectableSubsetParameter>("meshmotion"))->addIntoDictionaries(dictionaries);
  BoundaryCondition::addIntoDictionaries(dictionaries);
}

void WallBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
    Parameters p(ps_);
    
    multiphaseBC::multiphaseBCPtr phasefractions = 
        multiphaseBC::multiphaseBC::create( ps_.get<SelectableSubsetParameter>("phasefractions") );

    HeatBC::HeatBCPtr heattransfer =
        HeatBC::HeatBC::create( ps_.get<SelectableSubsetParameter>("heattransfer") );

    MeshMotionBC::MeshMotionBCPtr meshmotion =
        MeshMotionBC::MeshMotionBC::create( ps_.get<SelectableSubsetParameter>("meshmotion") );

    BoundaryCondition::addIntoFieldDictionaries(dictionaries);
    
    for (const FieldList::value_type& field: OFcase().fields())
    {
        OFDictData::dict& BC = dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
                               .subDict("boundaryField").subDict(patchName_);

        // velocity
        if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
        {
            if (p.rotating)
            {
                BC["type"]=OFDictData::data("rotatingWallVelocity");
                BC["origin"]=OFDictData::to_OF(p.CofR);
                double om=norm(p.wallVelocity, 2);
                BC["axis"]=OFDictData::to_OF(p.wallVelocity/om);
                BC["omega"]=lexical_cast<string>(om);
            }
            else
            {
                BC["type"]=OFDictData::data("movingWallVelocity");
                BC["value"]=OFDictData::data("uniform "+OFDictData::to_OF(p.wallVelocity));
            }
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
        }

        // turbulence quantities, should be handled by turbulence model
        else if (
            ( (field.first=="k") || (field.first=="omega") || (field.first=="epsilon") || (field.first=="nut") || (field.first=="nuSgs") || (field.first=="nuTilda") || (field.first=="alphat") )
            &&
            (get<0>(field.second)==scalarField)
        )
        {
            OFcase().get<turbulenceModel>("turbulenceModel")->addIntoFieldDictionary(field.first, field.second, BC, p.roughness_z0);
        }

        else
        {
            bool handled = false;

            handled = meshmotion->addIntoFieldDictionary(field.first, field.second, BC) || handled;
            handled = phasefractions->addIntoFieldDictionary ( field.first, field.second, BC ) || handled;
            handled = heattransfer->addIntoFieldDictionary ( field.first, field.second, BC ) || handled;

            if (!handled)
            {
                //throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
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

