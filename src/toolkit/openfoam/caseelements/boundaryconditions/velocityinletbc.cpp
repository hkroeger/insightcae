#include "velocityinletbc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

namespace insight {




defineType(VelocityInletBC);
addToFactoryTable(BoundaryCondition, VelocityInletBC);
addToStaticFunctionTable(BoundaryCondition, VelocityInletBC, defaultParameters);




VelocityInletBC::VelocityInletBC
(
  OpenFOAMCase& c,
  const std::string& patchName,
  const OFDictData::dict& boundaryDict,
  const ParameterSet& ps
)
: BoundaryCondition(c, patchName, boundaryDict, ps),
  ps_(ps)
{
 BCtype_="patch";
}




void VelocityInletBC::setField_p(OFDictData::dict& BC, OFdicts&) const
{
  BC["type"]=OFDictData::data("zeroGradient");
}




void VelocityInletBC::setField_U(OFDictData::dict& BC, OFdicts& dictionaries) const
{
    Parameters p ( ps_ );
    FieldData(p.velocity).setDirichletBC(BC, dictionaries);
//   FieldData(ps_.get<SelectableSubsetParameter>("velocity")()).setDirichletBC(BC);
}




void VelocityInletBC::addIntoFieldDictionaries ( OFdicts& dictionaries) const
{
    Parameters p ( ps_ );

    turbulenceBC::turbulenceBCPtr turbulence =
        turbulenceBC::turbulenceBC::create ( ps_.get<SelectableSubsetParameter> ( "turbulence" ) );
    multiphaseBC::multiphaseBCPtr phasefractions =
        multiphaseBC::multiphaseBC::create ( ps_.get<SelectableSubsetParameter> ( "phasefractions" ) );

    FieldData velocity(ps_.getSubset("velocity")), T(ps_.getSubset("T")), rho(ps_.getSubset("rho"));

    BoundaryCondition::addIntoFieldDictionaries ( dictionaries );
    phasefractions->addIntoDictionaries ( dictionaries );

    for ( const FieldList::value_type& field: OFcase().fields() ) {
        OFDictData::dict& BC=dictionaries.addFieldIfNonexistent ( "0/"+field.first, field.second )
                             .subDict ( "boundaryField" ).subDict ( patchName_ );
        if ( ( field.first=="U" ) && ( get<0> ( field.second ) ==vectorField ) ) {
            setField_U ( BC, dictionaries );
        }

        else if (
            ( field.first=="p" ) && ( get<0> ( field.second ) ==scalarField )
        ) {
            setField_p ( BC, dictionaries );
        } else if (
            ( field.first=="T" )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            T.setDirichletBC ( BC, dictionaries );
//       BC["type"]=OFDictData::data("fixedValue");
//       BC["value"]="uniform "+lexical_cast<string>(p_.T());
        } else if (isPrghPressureField(field)) {
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
            rho.setDirichletBC ( BC, dictionaries );
        } else if ( ( field.first=="k" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_k ( BC, velocity.representativeValueMag() );
        } else if ( ( field.first=="omega" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_omega ( BC, velocity.representativeValueMag() );
        } else if ( ( field.first=="epsilon" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_epsilon ( BC, velocity.representativeValueMag() );
        } else if ( ( field.first=="nut" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "calculated" );
            BC["value"]="uniform "+boost::lexical_cast<std::string> ( 1e-10 );
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



} // namespace insight
