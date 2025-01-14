#include "mappedvelocityinletbc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

namespace insight {




defineType(MappedVelocityInletBC);
addToFactoryTable(BoundaryCondition, MappedVelocityInletBC);
addToStaticFunctionTable(BoundaryCondition, MappedVelocityInletBC, defaultParameters);




MappedVelocityInletBC::MappedVelocityInletBC
(
  OpenFOAMCase& c,
  const std::string& patchName,
  const OFDictData::dict& boundaryDict,
  ParameterSetInput ip
)
: BoundaryCondition(
          c, patchName,
          boundaryDict,
          ip.forward<Parameters>() )
{
 BCtype_="patch";
}




void MappedVelocityInletBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
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
        bndDict["offset"]=OFDictData::vector3(p().distance);
    //bndDict["transform"]= "rotational";
//   }
//   else
//   {
//   }
}




void MappedVelocityInletBC::addIntoFieldDictionaries ( OFdicts& dictionaries ) const
{
    multiphaseBC::multiphaseBCPtr phasefractions =
        p().phasefractions;

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
            BC["average"]=OFDictData::vector3(p().average);
            BC["value"]=OFDictData::data ( "uniform ( 0 0 0 )" );
        } else if (
            ( field.first=="T" )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]="uniform "+boost::lexical_cast<std::string> ( p().T );
        } else if (
            ( ( field.first=="p" ) || isPrghPressureField(field) )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            BC["type"]=OFDictData::data ( "zeroGradient" );
        } else if ( ( field.first=="rho" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]=OFDictData::data ( "uniform "+boost::lexical_cast<std::string> ( p().rho ) );
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



} // namespace insight
