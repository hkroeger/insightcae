#include "suctioninletbc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

namespace insight {


defineType(SuctionInletBC);
addToFactoryTable(BoundaryCondition, SuctionInletBC);
addToStaticFunctionTable(BoundaryCondition, SuctionInletBC, defaultParameters);




SuctionInletBC::SuctionInletBC
(
  OpenFOAMCase& c,
  const std::string& patchName,
  const OFDictData::dict& boundaryDict,
  ParameterSetInput ip
)
: BoundaryCondition(
          c, patchName,
          boundaryDict,
          ip.forward<Parameters>())
{
 BCtype_="patch";
}




void SuctionInletBC::addIntoFieldDictionaries ( OFdicts& dictionaries ) const
{
    multiphaseBC::multiphaseBCPtr phasefractions =
        p().phasefractions;

    BoundaryCondition::addIntoFieldDictionaries ( dictionaries );

    bool compressible_case = this->OFcase().isCompressible();


    phasefractions->addIntoDictionaries ( dictionaries );

    for ( const FieldList::value_type& field: OFcase().fields() ) {
        OFDictData::dict& BC=dictionaries.addFieldIfNonexistent ( "0/"+field.first, field.second )
                             .subDict ( "boundaryField" ).subDict ( patchName_ );
        if ( ( field.first=="U" ) && ( get<0> ( field.second ) ==vectorField ) ) {

          if (const auto *normal = boost::get<Parameters::inletBehaviour_normal_type>(
                    &p().inletBehaviour))
          {
            BC["type"]=OFDictData::data ( "pressureInletOutletVelocity" );
          }
          else if (const auto *directed =
                     boost::get<Parameters::inletBehaviour_directed_type>(
                         &p().inletBehaviour))
          {
            BC["type"]=OFDictData::data ( "pressureDirectedInletOutletVelocity" );
            double dmag=arma::norm(directed->inflowDirection,2);
            if (dmag<1e-10)
              throw insight::Exception("SuctionInletBC: direction vector magnitude must not be zero!");
            BC["inletDirection"]=OFDictData::data ( "uniform " + OFDictData::to_OF(directed->inflowDirection/dmag) );
          }
          BC["value"]=OFDictData::data ( "uniform ( 0 0 0 )" );
        } else if (
            ( field.first=="T" )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            BC["type"]=OFDictData::data ( "inletOutletTotalTemperature" );
            BC["inletValue"]="uniform "+boost::lexical_cast<std::string> ( p().T );
            BC["T0"]="uniform "+boost::lexical_cast<std::string> ( p().T );
            BC["U"]=OFDictData::data ( p().UName );
            BC["phi"]=OFDictData::data ( p().phiName );
            BC["psi"]=OFDictData::data ( p().psiName );
            BC["gamma"]=OFDictData::data ( p().gamma );
            BC["value"]="uniform "+boost::lexical_cast<std::string> ( p().T );
        } else if (
            ( ( field.first=="p" ) || isPrghPressureField(field) )
            &&
            ( get<0> ( field.second ) ==scalarField )
        ) {
            BC["type"]=OFDictData::data ( "totalPressure" );
            BC["p0"]=OFDictData::data ( "uniform "+boost::lexical_cast<std::string> ( p().pressure ) );
            BC["U"]=OFDictData::data ( p().UName );
            BC["phi"]=OFDictData::data ( p().phiName );
            BC["rho"]=OFDictData::data ( p().rhoName );
            BC["psi"]=OFDictData::data ( p().psiName );
            BC["gamma"]=OFDictData::data ( p().gamma );
            BC["value"]=OFDictData::data ( "uniform "+boost::lexical_cast<std::string> ( p().pressure ) );
        }
        else if ( ( field.first=="rho" ) && ( get<0> ( field.second ) ==scalarField ) )
          {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]=OFDictData::data ( "uniform "
                                           +boost::lexical_cast<std::string> ( p().rho ) );
          }
        else if ( ( field.first=="k" ) && ( get<0> ( field.second ) ==scalarField ) )
          {
            BC["type"]=OFDictData::data ( "turbulentIntensityKineticEnergyInlet" );
            BC["intensity"]=p().turb_I;
            BC["value"]=OFDictData::data ( "uniform "+boost::lexical_cast<std::string> ( 0.1 ) );
          }
        else if ( ( field.first=="omega" ) && ( get<0> ( field.second ) ==scalarField ) )
          {
            BC["type"]=
                (compressible_case&&(OFversion()<300) ? "compressible::" : "")
                + std::string("turbulentMixingLengthFrequencyInlet");
            BC["mixingLength"]=p().turb_L;
            BC["value"]=OFDictData::data ( "uniform "+boost::lexical_cast<std::string> ( 1.0 ) );
          }
        else if ( ( field.first=="epsilon" ) && ( get<0> ( field.second ) ==scalarField ) )
          {
            BC["type"]=
                (compressible_case&&(OFversion()<300) ? "compressible::" : "")
                + std::string("turbulentMixingLengthDissipationRateInlet");
            BC["mixingLength"]=p().turb_L;
            BC["value"]=OFDictData::data ( "uniform "+boost::lexical_cast<std::string> ( 1.0 ) );
          }
        else if ( (
                    ( field.first=="nut" ) ||
                    ( field.first=="nuSgs" )
                   ) && ( get<0> ( field.second ) ==scalarField ) )
          {
            BC["type"]=OFDictData::data ( "calculated" );
            BC["value"]=OFDictData::data ( "uniform "+boost::lexical_cast<std::string> ( 0.0 ) );
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




} // namespace insight
