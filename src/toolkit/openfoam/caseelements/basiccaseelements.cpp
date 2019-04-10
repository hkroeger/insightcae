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


cellSetOption_Selection::cellSetOption_Selection(const Parameters& p)
  : p_(p)
{}


void cellSetOption_Selection::insertSelection(OFDictData::dict& d)
{
  if (const auto* all = boost::get<Parameters::selection_all_type>(&p_.selection))
    {
      d["selectionMode"]="all";
    }
  else if (const auto* cz = boost::get<Parameters::selection_cellZone_type>(&p_.selection))
    {
      d["selectionMode"]="cellZone";
      d["cellZone"]=cz->zoneName;
    }
  else if (const auto* cs = boost::get<Parameters::selection_cellSet_type>(&p_.selection))
    {
      d["selectionMode"]="cellSet";
      d["cellSet"]=cs->setName;
    }
  else
    {
      throw insight::Exception("Internal error: unhandled selection");
    }
}

    
    
    
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
  for (size_t i=0; i<3; i++) gv.push_back(p_.g(i));
  g["value"]=gv;
}

ParameterSet gravity::defaultParameters()
{
    return Parameters::makeDefault();
}
std::string gravity::category()
{
  return "Body Force";
}

bool gravity::isUnique() const
{
  return true;
}


defineType(mirrorMesh);
addToOpenFOAMCaseElementFactoryTable(mirrorMesh);

mirrorMesh::mirrorMesh( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "mirrorMesh"),
  p_(ps)
{
}

void mirrorMesh::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& mmd=dictionaries.addDictionaryIfNonexistent("system/mirrorMeshDict");

  mmd["planeTolerance"]=p_.planeTolerance;

  if (const Parameters::plane_pointAndNormal_type* pn =
      boost::get<Parameters::plane_pointAndNormal_type>(&p_.plane))
    {
      mmd["planeType"]="pointAndNormal";
      OFDictData::dict d;
      d["basePoint"]=OFDictData::vector3(pn->p0);
      d["normalVector"]=OFDictData::vector3(pn->normal);
      mmd["pointAndNormalDict"]=d;
    }
  else if (const Parameters::plane_threePoint_type* pt =
           boost::get<Parameters::plane_threePoint_type>(&p_.plane))
    {
      mmd["planeType"]="embeddedPoints";
      OFDictData::dict d;
      d["point1"]=OFDictData::vector3(pt->p0);
      d["point2"]=OFDictData::vector3(pt->p1);
      d["point3"]=OFDictData::vector3(pt->p2);
      mmd["embeddedPointsDict"]=d;
    }
  else
    throw insight::Exception("Internal error: Unhandled selection!");
}

bool mirrorMesh::isUnique() const
{
  return true;
}





defineType(setFieldsConfiguration);
addToOpenFOAMCaseElementFactoryTable(setFieldsConfiguration);



setFieldsConfiguration::setFieldsConfiguration( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "setFieldsConfiguration"),
  p_(ps)
{
}


void setFieldsConfiguration::addIntoDictionaries(OFdicts& dictionaries) const
{

    OFDictData::dict& sFD
      = dictionaries.addDictionaryIfNonexistent("system/setFieldsDict");

    OFDictData::list dfvs;
    for (const Parameters::defaultValues_default_type& dfv: p_.defaultValues)
    {
      if (const auto * sdfv = boost::get<Parameters::defaultValues_default_scalar_type>(&dfv))
        {
          dfvs.push_back(str(format("volScalarFieldValue %s %g\n") % sdfv->name % sdfv->value));
        }
      else if (const auto * vdfv = boost::get<Parameters::defaultValues_default_vector_type>(&dfv))
        {
          dfvs.push_back(str(format("volVectorFieldValue %s %s\n") % vdfv->name % OFDictData::to_OF(vdfv->value)));
        }
      else
        throw insight::Exception("Internal error: Unhandled selection!");
    }
    sFD["defaultFieldValues"]=dfvs;

    OFDictData::list rs;
    for (const Parameters::regionSelectors_default_type& r: p_.regionSelectors)
    {
      if (const auto * box = boost::get<Parameters::regionSelectors_default_box_type>(&r))
        {
          OFDictData::list vl;
          for (const Parameters::regionSelectors_default_box_type::regionValues_default_type& bv: box->regionValues)
          {
            if (const auto * s = boost::get<Parameters::regionSelectors_default_box_type::regionValues_default_scalar_type>(&bv))
              {
                vl.push_back(str(format("volScalarFieldValue %s %g\n") % s->name % s->value));
              }
            else if (const auto * v = boost::get<Parameters::regionSelectors_default_box_type::regionValues_default_vector_type>(&bv))
              {
                vl.push_back(str(format("volVectorFieldValue %s %s\n") % v->name % OFDictData::to_OF(v->value)));
              }
            else
              throw insight::Exception("Internal error: Unhandled type selection!");

            OFDictData::dict fs;
            fs["box"]=str(format("%s %s") % OFDictData::to_OF(box->p0) % OFDictData::to_OF(box->p1) );
            fs["fieldValues"]=vl;

            if (box->selectcells)
              {
                rs.push_back("boxToCell");
                rs.push_back(fs);
              }

            if (box->selectfaces)
              {
                rs.push_back("boxToFace");
                rs.push_back(fs);
              }
          }
        }
      else
        throw insight::Exception("Internal error: Unhandled region selection!");
    }
    sFD["regions"]=rs;
}


bool setFieldsConfiguration::isUnique() const
{
  return true;
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




defineType(fixedValueConstraint);
addToOpenFOAMCaseElementFactoryTable(fixedValueConstraint);

fixedValueConstraint::fixedValueConstraint( OpenFOAMCase& c, const ParameterSet& ps)
: OpenFOAMCaseElement(c, ""),
  p_(ps)
{
  name_="fixedValueConstraint"+p_.name;
}

void fixedValueConstraint::addIntoDictionaries ( OFdicts& dictionaries ) const
{
//  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
//  controlDict.getList("libs").insertNoDuplicate( "\"libvolumeDragfvOption.so\"" );

  OFDictData::dict cd;
  cd["active"]=true;
  cd["selectionMode"]="cellZone";
  cd["cellZone"]=p_.zoneName;
  OFDictData::dict fvd;

  if (const auto* cs = boost::get<Parameters::value_scalar_type>(&p_.value))
  {
    cd["type"]="scalarFixedValueConstraint";
    fvd[p_.fieldName]=cs->value;
  }
  else if (const auto* cs = boost::get<Parameters::value_vector_type>(&p_.value))
  {
    cd["type"]="vectorFixedValueConstraint";
    fvd[p_.fieldName]=OFDictData::vector3(cs->value);
  }

  cd["fieldValues"]=fvd;

  OFDictData::dict& fvOptions=dictionaries.addDictionaryIfNonexistent("system/fvOptions");
  fvOptions[p_.name]=cd;
}




defineType(source);
addToOpenFOAMCaseElementFactoryTable(source);

source::source( OpenFOAMCase& c, const ParameterSet& ps)
: OpenFOAMCaseElement(c, ""),
  p_(ps)
{
  name_="source"+p_.name;
}

void source::addIntoDictionaries ( OFdicts& dictionaries ) const
{
//  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
//  controlDict.getList("libs").insertNoDuplicate( "\"libvolumeDragfvOption.so\"" );

  OFDictData::dict cd;
  cd["active"]=true;
  cd["selectionMode"]="cellZone";
  cd["cellZone"]=p_.zoneName;
  cd["volumeMode"]= p_.volumeMode == Parameters::specific ? "specific" : "absolute";

  OFDictData::dict ijr;
  if (const auto* cs = boost::get<Parameters::value_scalar_type>(&p_.value))
  {
    cd["type"]="scalarSemiImplicitSource";
    OFDictData::list vals;
    vals.push_back( cs->value_const );
    vals.push_back( cs->value_lin );
    ijr[p_.fieldName]=vals;
  }
  else if (const auto* cs = boost::get<Parameters::value_vector_type>(&p_.value))
  {
    cd["type"]="vectorSemiImplicitSource";
    OFDictData::list vals;
    vals.push_back( OFDictData::vector3(cs->value_const) );
    vals.push_back( OFDictData::vector3(cs->value_lin) );
    ijr[p_.fieldName]=vals;
  }

  cd["injectionRateSuSp"]=ijr;

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
  c.addField(p_.fieldname, 	FieldInfo(scalarField, 	dimless, 	FieldValue({p_.internal}), volField ) );
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

    if (const auto* fo = boost::get<Parameters::scheme_firstorder_type>(&p_.scheme))
      {
        divSchemes["div(phi,"+p_.fieldname+")"]="Gauss upwind";
      }
    else if (const auto* so = boost::get<Parameters::scheme_secondorder_type>(&p_.scheme))
      {
        if (p_.underrelax < 1.)
          {
            OFDictData::dict& gradSchemes = fvSchemes.addSubDictIfNonexistent("gradSchemes");

            divSchemes["div(phi,"+p_.fieldname+")"]="Gauss linearUpwind grad("+p_.fieldname+")";
            gradSchemes["grad(T)"]="cellLimited Gauss linear 1";
          }
        else
          {
            if (so->bounded01)
              divSchemes["div(phi,"+p_.fieldname+")"]="Gauss limitedLinear01 1";
            else
              divSchemes["div(phi,"+p_.fieldname+")"]="Gauss limitedLinear 1";
          }
      }


    OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
    OFDictData::dict& solvers=fvSolution.subDict("solvers");
    solvers[p_.fieldname]=smoothSolverSetup(1e-6, 0.);

    OFDictData::dict& relax=fvSolution.addSubDictIfNonexistent("relaxationFactors");
    if (OFversion()<210)
    {
      relax[p_.fieldname]=p_.underrelax;
    }
    else
    {
      OFDictData::dict& eqnRelax=relax.addSubDictIfNonexistent("equations");
      eqnRelax[p_.fieldname]=p_.underrelax;
    }
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
    if (OFversion()>=400)
    {
        coeffs["selectionMode"]="all";
    }
    else
    {
        fod["selectionMode"]="all";
    }
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




bool transportModel::isUnique() const
{
  return true;
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

bool dynamicMesh::isUnique() const
{
  return true;
}


velocityTetFEMMotionSolver::velocityTetFEMMotionSolver(OpenFOAMCase& c)
: dynamicMesh(c),
  tetFemNumerics_(c)
{
  c.addField("motionU", FieldInfo(vectorField, 	dimVelocity, 		FieldValue({0.0, 0.0, 0.0}), tetField ) );
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
    else if ( Parameters::motion_oscillatingRotating_type* ro = boost::get<Parameters::motion_oscillatingRotating_type>(&p.motion) )
    {
        sbc["solidBodyMotionFunction"]="oscillatingRotatingMotion";
        OFDictData::dict rmc;
        rmc["origin"]=OFDictData::vector3(ro->origin);
        rmc["omega"]=ro->omega;
        rmc["amplitude"]=OFDictData::vector3(ro->amplitude);
        sbc["oscillatingRotatingMotionCoeffs"]=rmc;
    }
    else
      throw insight::Exception("Internal error: Unhandled selection!");

    dynamicMeshDict["solidBodyCoeffs"]=sbc;
}




defineType(rigidBodyMotionDynamicMesh);
addToOpenFOAMCaseElementFactoryTable(rigidBodyMotionDynamicMesh);



rigidBodyMotionDynamicMesh::rigidBodyMotionDynamicMesh( OpenFOAMCase& c, const ParameterSet& ps )
: dynamicMesh(c),
  ps_(ps)
{
}

void rigidBodyMotionDynamicMesh::addFields( OpenFOAMCase& c ) const
{
  c.addField
  (
      "pointDisplacement",
       FieldInfo(vectorField, 	dimLength, 	FieldValue({0, 0, 0}), pointField )
  );
}

void rigidBodyMotionDynamicMesh::addIntoDictionaries(OFdicts& dictionaries) const
{
    Parameters p(ps_);

    OFDictData::dict& dynamicMeshDict
      = dictionaries.addDictionaryIfNonexistent("constant/dynamicMeshDict");

    dynamicMeshDict["dynamicFvMesh"]="dynamicMotionSolverFvMesh";
    dynamicMeshDict["solver"]="rigidBodyMotion";

    OFDictData::list libl;
    libl.push_back("\"librigidBodyMeshMotion.so\"");
    dynamicMeshDict["motionSolverLibs"]=libl;

    OFDictData::dict rbmc;
    rbmc["report"]=true;

    OFDictData::dict sc;
     sc["type"]="Newmark";
    rbmc["solver"]=sc;

    if (const Parameters::rho_field_type* rhof = boost::get<Parameters::rho_field_type>(&p.rho))
      {
        rbmc["rho"]=rhof->fieldname;
      }
    else if (const Parameters::rho_constant_type* rhoc = boost::get<Parameters::rho_constant_type>(&p.rho))
      {
        rbmc["rho"]="rhoInf";
        rbmc["rhoInf"]=rhoc->rhoInf;
      }

    rbmc["accelerationRelaxation"]=0.4;

    OFDictData::dict bl;
    for (const Parameters::bodies_default_type& body: p.bodies)
    {
      OFDictData::dict bc;

      bc["type"]="rigidBody";
      bc["parent"]="root";
      bc["centreOfMass"]=OFDictData::vector3(0,0,0);
      bc["mass"]=body.mass;
      bc["inertia"]=boost::str(boost::format("(%g 0 0 %g 0 %g)") % body.Ixx % body.Iyy % body.Izz);
      bc["transform"]=boost::str(boost::format("(1 0 0 0 1 0 0 0 1) (%g %g %g)")
                                 % body.centreOfMass(0) % body.centreOfMass(1) % body.centreOfMass(2) );

      OFDictData::list jl;
      for (const Parameters::bodies_default_type::translationConstraint_default_type& tc:
                    body.translationConstraint)
      {
        std::string code;
        if (tc==Parameters::bodies_default_type::translationConstraint_default_type::Px) code="Px";
        else if (tc==Parameters::bodies_default_type::translationConstraint_default_type::Py) code="Py";
        else if (tc==Parameters::bodies_default_type::translationConstraint_default_type::Pz) code="Pz";
        else if (tc==Parameters::bodies_default_type::translationConstraint_default_type::Pxyz) code="Pxyz";
        else throw insight::Exception("internal error: unhandled value!");
        OFDictData::dict d;
         d["type"]=code;
        jl.push_back(d);
      }
      for (const Parameters::bodies_default_type::rotationConstraint_default_type& rc:
                    body.rotationConstraint)
      {
        std::string code;
        if (rc==Parameters::bodies_default_type::rotationConstraint_default_type::Rx) code="Rx";
        else if (rc==Parameters::bodies_default_type::rotationConstraint_default_type::Ry) code="Ry";
        else if (rc==Parameters::bodies_default_type::rotationConstraint_default_type::Rz) code="Rz";
        else if (rc==Parameters::bodies_default_type::rotationConstraint_default_type::Rxyz) code="Rxyz";
        else throw insight::Exception("internal error: unhandled value!");
        OFDictData::dict d;
         d["type"]=code;
        jl.push_back(d);
      }
      OFDictData::dict jc;
       jc["type"]="composite";
       jc["joints"]=jl;
      bc["joint"]=jc;

      OFDictData::list pl;
      std::copy(body.patches.begin(), body.patches.end(), std::back_inserter(pl));
      bc["patches"]=pl;

      bc["innerDistance"]=body.innerDistance;
      bc["outerDistance"]=body.outerDistance;

      bl[body.name]=bc;
    }
    rbmc["bodies"]=bl;

    OFDictData::dict rc;
     // empty
    rbmc["restraints"]=rc;

    dynamicMeshDict["rigidBodyMotionCoeffs"]=rbmc;
}






defineType(porousZone);
addToOpenFOAMCaseElementFactoryTable(porousZone);

porousZone::porousZone( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, ""),
  p_(ps)
{
    name_="porousZone"+p_.name;
}

void porousZone::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& porosityProperties
    = dictionaries.addDictionaryIfNonexistent("constant/porosityProperties");

  OFDictData::dict pc;

  pc["type"]="DarcyForchheimer";
  pc["active"]=true;
  pc["cellZone"]=p_.name;
    OFDictData::dict dfc;
    dfc["d"]=OFDictData::vector3(p_.d);
    dfc["f"]=OFDictData::vector3(p_.f);

    OFDictData::dict cs;
     cs["type"]="cartesian";
     cs["origin"]=OFDictData::vector3(vec3(0,0,0));
     OFDictData::dict cr;
      cr["type"]="axesRotation";
      cr["e1"]=OFDictData::vector3(p_.direction_x);
      cr["e2"]=OFDictData::vector3(p_.direction_y);
     cs["coordinateRotation"]=cr;
    dfc["coordinateSystem"]=cs;
  pc["DarcyForchheimerCoeffs"]=dfc;

  porosityProperties[p_.name]=pc;

}





defineType(limitQuantities);
addToOpenFOAMCaseElementFactoryTable(limitQuantities);

limitQuantities::limitQuantities( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, ""),
  p_(ps)
{
    name_="limitQuantities"+p_.name;
}

void limitQuantities::addIntoDictionaries(OFdicts& dictionaries) const
{
//  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
//  controlDict.getList("libs").insertNoDuplicate( "\"libvolumeDragfvOption.so\"" );
  OFDictData::dict& fvOptions=dictionaries.addDictionaryIfNonexistent("system/fvOptions");

  cellSetOption_Selection sel(p_.cells);

  if (const auto* limT = boost::get<Parameters::limitTemperature_limit_type>(&p_.limitTemperature))
    {
      OFDictData::dict cdT;
      cdT["type"]="limitTemperature";
      cdT["active"]=true;

      OFDictData::dict c;
      sel.insertSelection(c);
      c["min"]=limT->min;
      c["max"]=limT->max;
      cdT["limitTemperatureCoeffs"]=c;

      fvOptions[p_.name+"_temp"]=cdT;
    }

  if (const auto* limU = boost::get<Parameters::limitVelocity_limit_type>(&p_.limitVelocity))
    {
      OFDictData::dict cdU;
      cdU["type"]="limitVelocity";
      cdU["active"]=true;

      OFDictData::dict c;
      sel.insertSelection(c);
      c["max"]=limU->max;
      cdU["limitVelocityCoeffs"]=c;

      fvOptions[p_.name+"_vel"]=cdU;
    }

  for (const auto& i: p_.limitFields)
    {
      string type;
      switch (i.type)
        {
         case Parameters::limitFields_default_type::scalar:
           type="scalarlimitFields";
          break;
        }

      OFDictData::dict cd;
      cd["type"]=type;
      cd["active"]=true;

      OFDictData::dict c;
      sel.insertSelection(c);
      c["fieldName"]=i.fieldName;
      c["max"]=i.max;
      c["min"]=i.min;
      cd[type+"Coeffs"]=c;

      fvOptions[p_.name+"_"+i.fieldName]=cd;
    }

}






defineType(customDictEntries);
addToOpenFOAMCaseElementFactoryTable(customDictEntries);

customDictEntries::customDictEntries( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, ""),
  p_(ps)
{
    name_="customDictEntries";
}

OFDictData::dict& getOrCreateSubDict(OFDictData::dict& d, std::string path)
{
  if (path.empty())
    {
      return d;
    }
  else
    {
      auto i = path.find('/');
      if (i==string::npos)
        {
          return d.addSubDictIfNonexistent(path);
        }
      else
        {
          string k = path.substr(0, i);
          string p = path.substr(i+1);
          return getOrCreateSubDict( d.addSubDictIfNonexistent(k), p );
        }
    }
}

void customDictEntries::addIntoDictionaries(OFdicts& dictionaries) const
{
  for (const auto& e: p_.entries)
    {
      OFDictData::dict& dict
        = dictionaries.addDictionaryIfNonexistent(e.dict);

      string path, key;
      auto i = e.path.rfind('/');
      if (i==string::npos)
        {
          path="";
          key=e.path;
        }
      else
        {
          path = e.path.substr(0, i);
          key = e.path.substr(i+1);
        }

      OFDictData::dict& parent = getOrCreateSubDict(dict, path);
      parent[key]=e.value;
    }

  for (const auto& e: p_.appendList)
    {
      OFDictData::dict& dict
        = dictionaries.addDictionaryIfNonexistent(e.dict);

      string path, key;
      auto i = e.path.rfind('/');
      if (i==string::npos)
        {
          path="";
          key=e.path;
        }
      else
        {
          path = e.path.substr(0, i);
          key = e.path.substr(i+1);
        }

      OFDictData::dict& parent = getOrCreateSubDict(dict, path);
      parent.getList(key).push_back(e.value);
    }


}







defineType(copyFiles);
addToOpenFOAMCaseElementFactoryTable(copyFiles);

copyFiles::copyFiles( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, ""),
  p_(ps)
{
    name_="copyFiles";
}

namespace fs = boost::filesystem;

void copyDirectoryRecursively(const fs::path& sourceDir, const fs::path& destinationDir)
{
    if (!fs::exists(sourceDir) || !fs::is_directory(sourceDir))
    {
        throw std::runtime_error("Source directory " + sourceDir.string() + " does not exist or is not a directory");
    }
    if (fs::exists(destinationDir))
    {
        throw std::runtime_error("Destination directory " + destinationDir.string() + " already exists");
    }
    if (!fs::create_directory(destinationDir))
    {
        throw std::runtime_error("Cannot create destination directory " + destinationDir.string());
    }

    for (const auto& dirEnt : boost::make_iterator_range(fs::recursive_directory_iterator{sourceDir}, {}))
    {
        const auto& path = dirEnt.path();
        auto relativePathStr = path.string();
        boost::replace_first(relativePathStr, sourceDir.string(), "");
        fs::copy(path, destinationDir / relativePathStr);
    }
}

void copyFiles::addIntoDictionaries(OFdicts&) const
{
}

void copyFiles::modifyFilesOnDiskBeforeDictCreation ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const
{
  for (const auto& f: p_.files)
    {
      path src = f.source;
      path targ = location / f.target;

      if (!exists(src))
        throw insight::Exception("Source file/directory "+src.string()+" does not exist!");

      if (!exists(targ.parent_path()))
        create_directories(targ.parent_path());

      if (boost::filesystem::is_directory(src))
        copyDirectoryRecursively(src, targ);
      else
        boost::filesystem::copy_file(src, targ);

    }
}







defineType(SRFoption);
addToOpenFOAMCaseElementFactoryTable(SRFoption);

SRFoption::SRFoption( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, ""),
  p_(ps)
{
    name_="SRFoption";
}



void SRFoption::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict.getList("libs").insertNoDuplicate( "\"libSRFoption.so\"" );

  OFDictData::dict cd;
  cd["type"]="SRFoption";
  OFDictData::dict& fvOptions=dictionaries.addDictionaryIfNonexistent("system/fvOptions");
  fvOptions[name()]=cd;

  OFDictData::dict& SRFProperties=dictionaries.addDictionaryIfNonexistent("constant/SRFProperties");
  SRFProperties["SRFModel"]="rpm";
  SRFProperties["origin"]=OFDictData::vector3(p_.origin);
  SRFProperties["axis"]=OFDictData::vector3( p_.axis/arma::norm(p_.axis,2) );

  OFDictData::dict& rpmCoeffs=SRFProperties.addSubDictIfNonexistent("rpmCoeffs");
  rpmCoeffs["rpm"]=p_.rpm;
}


}


