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
    const ParameterSet& ps
)
    : BoundaryCondition ( c, patchName, boundaryDict, ps ),
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
              if (OFversion()<170)
                BC["flowRate"]=vf->value;
              else
                BC["volumetricFlowRate"]=vf->value;
            }
            BC["value"]=OFDictData::data ( "uniform ( 0 0 0 )" );
        } else if (
            ( field.first=="T" )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]="uniform "+boost::lexical_cast<std::string> ( p.T );
//            BC["type"]=OFDictData::data ( "inletOutletTotalTemperature" );
//            BC["inletValue"]="uniform "+lexical_cast<string> ( p.T );
//            BC["T0"]="uniform "+lexical_cast<string> ( p.T );
//            BC["U"]=OFDictData::data ( p.UName );
//            BC["phi"]=OFDictData::data ( p.phiName );
//            BC["psi"]=OFDictData::data ( p.psiName );
//            BC["gamma"]=OFDictData::data ( p.gamma );
//            BC["value"]="uniform "+lexical_cast<string> ( p.T );
        } else if (isPrghPressureField(field)) {
            if ( OFversion() >=210 ) {
                BC["type"]=OFDictData::data ( "fixedFluxPressure" );
            } else {
                BC["type"]=OFDictData::data ( "buoyantPressure" );
            }
//       BC["type"]=OFDictData::data("calculated");
//       BC["value"]=OFDictData::data("uniform 0");
        } else if ( ( field.first=="rho" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]=OFDictData::data ( "uniform "+boost::lexical_cast<std::string> ( p.rho ) );
        } else if ( ( field.first=="k" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_k ( BC, velocity );
        } else if ( ( field.first=="omega" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_omega ( BC, velocity );
        } else if ( ( field.first=="epsilon" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            turbulence->setDirichletBC_epsilon ( BC, velocity );
        } else if ( ( field.first=="nut" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "calculated" );
            BC["value"]="uniform "+boost::lexical_cast<std::string> ( 1e-10 );
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




} // namespace insight
