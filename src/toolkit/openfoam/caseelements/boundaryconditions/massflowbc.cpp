#include "massflowbc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"


namespace insight {



defineType(MassflowBC);
addToFactoryTable(BoundaryCondition, MassflowBC);
addToStaticFunctionTable(BoundaryCondition, MassflowBC, defaultParameters);




MassflowBC::MassflowBC
(
    OpenFOAMCase& c,
    const std::string& patchName,
    const OFDictData::dict& boundaryDict,
    ParameterSetInput ip
)
    : BoundaryCondition ( c, patchName, boundaryDict, ip.forward<Parameters>() )
{
    BCtype_="patch";
}




void MassflowBC::addIntoFieldDictionaries ( OFdicts& dictionaries ) const
{
    turbulenceBC::turbulenceBCPtr turbulence =
        p().turbulence;
    multiphaseBC::multiphaseBCPtr phasefractions =
        p().phasefractions;

    BoundaryCondition::addIntoFieldDictionaries ( dictionaries );
    phasefractions->addIntoDictionaries ( dictionaries );

    double velocity=1.0; // required for turbulence quantities. No better idea yet...
    for ( const FieldList::value_type& field: OFcase().fields() ) {
        OFDictData::dict& BC=dictionaries.addFieldIfNonexistent ( "0/"+field.first, field.second )
                             .subDict ( "boundaryField" ).subDict ( patchName_ );
        if ( ( field.first=="U" ) && ( boost::fusion::get<0> ( field.second ) ==vectorField ) ) {
            BC["type"]=OFDictData::data ( "flowRateInletVelocity" );
            if (const auto *mf = boost::get<Parameters::flowrate_massflow_type>(
                    &p().flowrate))
            {
                BC["rho"]=p().rhoName;
                BC["massFlowRate"]=mf->value;
            }
            else if (const auto *vf = boost::get<Parameters::flowrate_volumetric_type>(
                           &p().flowrate))
            {
              if (OFversion()<170)
                BC["flowRate"]=vf->value;
              else
                BC["volumetricFlowRate"]=vf->value;
            }
            BC["value"]=OFDictData::toUniformField( vec3Zero() );
        } else if (
            ( field.first=="T" )
            &&
            ( boost::fusion::get<0> ( field.second ) ==scalarField )
        ) {
            if (auto* Tstatic=boost::get<Parameters::temperature_staticTemperature_type>(
                    &p().temperature))
            {
                BC["type"]=OFDictData::data ( "inletOutlet" );
                BC["inletValue"]=OFDictData::toUniformField( Tstatic->T );
                BC["value"]=OFDictData::toUniformField( Tstatic->T );
            }
            else if (auto* Ttot=boost::get<Parameters::temperature_totalTemperature_type>(
                    &p().temperature))
            {
                BC["type"]=OFDictData::data ( "totalTemperature" );
                BC["gamma"]=1.4;
                BC["T0"]=OFDictData::toUniformField( Ttot->T0 );
                BC["value"]=OFDictData::toUniformField( Ttot->T0 );
            }
            else
                throw UnhandledSelection();
        } else if (isPrghPressureField(field)) {
            if ( OFversion() >=210 ) {
                BC["type"]=OFDictData::data ( "fixedFluxPressure" );
            } else {
                BC["type"]=OFDictData::data ( "buoyantPressure" );
            }
        } else if ( ( field.first=="rho" ) && ( boost::fusion::get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]=OFDictData::toUniformField( p().rho );
        } else if ( ( field.first=="k" ) && ( boost::fusion::get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_k ( BC, velocity );
        } else if ( ( field.first=="omega" ) && ( boost::fusion::get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_omega ( BC, velocity );
        } else if ( ( field.first=="epsilon" ) && ( boost::fusion::get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_epsilon ( BC, velocity );
        } else if ( ( field.first=="nut" ) && ( boost::fusion::get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "calculated" );
            BC["value"]=OFDictData::toUniformField( 1e-10 );
        } else if ( ( field.first=="nuTilda" ) && ( boost::fusion::get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_nuTilda ( BC, velocity );
        } else if ( ( field.first=="R" ) && ( boost::fusion::get<0> ( field.second ) ==symmTensorField ) ) {
            turbulence->setDirichletBC_R ( BC, velocity );
        } else if ( ( field.first=="nuSgs" ) && ( boost::fusion::get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]=OFDictData::toUniformField(1e-10);
        } else {
            if ( ! (
                        MeshMotionBC::passiveMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC )
                        ||
                        phasefractions->addIntoFieldDictionary ( field.first, field.second, BC )
                    ) )
            {
                BC["type"]=OFDictData::data ( "zeroGradient" );
            }
        }
    }
}




} // namespace insight
