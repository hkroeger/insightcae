#include "pressureoutletbc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

namespace insight {




defineType(PressureOutletBC);
addToFactoryTable(BoundaryCondition, PressureOutletBC);
addToStaticFunctionTable(BoundaryCondition, PressureOutletBC, defaultParameters);




PressureOutletBC::PressureOutletBC
(
  OpenFOAMCase& c,
  const std::string& patchName,
  const OFDictData::dict& boundaryDict,
  ParameterSetInput ip
)
: BoundaryCondition(
          c, patchName, boundaryDict,
          ip.forward<Parameters>())
{
 BCtype_="patch";
}




void PressureOutletBC::addIntoFieldDictionaries ( OFdicts& dictionaries ) const
{
    multiphaseBC::multiphaseBCPtr phasefractions =
        p().phasefractions;

    BoundaryCondition::addIntoFieldDictionaries ( dictionaries );
    phasefractions->addIntoDictionaries(dictionaries);

    if ( (boost::get<Parameters::behaviour_fixMeanValue_type>(&p().behaviour)) && ( OFversion() !=160 ) ) {
        OFDictData::dict& controlDict=dictionaries.lookupDict ( "system/controlDict" );
        controlDict.getList ( "libs" ).push_back ( "\"libfixedMeanValueBC.so\"" );
    }

    for ( const FieldList::value_type& field: OFcase().fields() ) {
        OFDictData::dict& BC=dictionaries.addFieldIfNonexistent ( "0/"+field.first, field.second )
                             .subDict ( "boundaryField" ).subDict ( patchName_ );

        if ( ( field.first=="U" ) && ( get<0> ( field.second ) ==vectorField ) ) {
            if ( p().prohibitInflow ) {
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
               ( ( field.first=="p" ) || isPrghPressureField(field) )
               &&
               ( get<0> ( field.second ) ==scalarField )
              )
        {
            if ( (field.first=="p") && OFcase().hasPrghPressureField() )
              {
                BC["type"]=OFDictData::data ( "calculated" );
                BC["value"]=OFDictData::data ( "uniform "+boost::lexical_cast<std::string> ( /*p.pressure*/ 1e5 ) );
              }
            else
              {
                if ( const auto* unif =
                      boost::get<Parameters::behaviour_uniform_type>(
                          &p().behaviour) )
                {
                    FieldData(unif->pressure).setDirichletBC(BC, dictionaries);
//                    BC["type"]=OFDictData::data ( "fixedValue" );
//                    BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( unif->pressure ) );
                }
                else if ( const auto* fixmean =
                           boost::get<Parameters::behaviour_fixMeanValue_type>(
                               &p().behaviour) )
                {
                    BC["type"]=OFDictData::data ( "fixedMeanValue" );
                    BC["meanValue"]=OFDictData::data ( fixmean->pressure );
                    BC["value"]=OFDictData::data ( "uniform "+boost::lexical_cast<std::string> ( fixmean->pressure ) );
                }
                else if ( const auto* tvu =
                           boost::get<Parameters::behaviour_timeVaryingUniform_type>(
                               &p().behaviour) )
                {
                    BC["type"]=OFDictData::data ( "timeVaryingUniformFixedValue" );
                    boost::filesystem::path filename = patchName_ + "_pressureOutlet.txt";
                    BC["fileName"]= "\"" + ( boost::filesystem::path("$FOAM_CASE")/filename ).string() + "\"";
                    BC["outOfBounds"]="clamp";
                    //BC["value"]=OFDictData::data ( "uniform "+lexical_cast<std::string> ( fixmean->pressure ) );

                    auto& sd = dictionaries.lookupDict(filename.string());
                    sd.no_header=true;
                    sd.isSequential=true;

                    OFDictData::list p_list;
                    BOOST_FOREACH(const auto& p, tvu->sequel)
                    {
                      OFDictData::list inst;
                      inst.push_back(p.time); inst.push_back(p.pressure);
                      p_list.push_back(inst);
                    }

                    sd[""]=p_list;
                }
                else if ( const auto* wt =
                           boost::get<Parameters::behaviour_waveTransmissive_type>(
                               &p().behaviour) )
                {
                    BC["type"]="waveTransmissive";
                    BC["psi"]=wt->psiName;
                    BC["rho"]=wt->rhoName;
                    BC["inletOutlet"]=false;
                    BC["correctSupercritical"]=false;
                    BC["gamma"]=wt->kappa;
                    BC["lInf"]=wt->L;
                    BC["fieldInf"]=OFDictData::data ( wt->pressure );
                    BC["value"]=OFDictData::data (
                        "uniform "
                        +boost::lexical_cast<std::string> ( wt->pressure ) );
                }
                else if ( const auto* wt =
                           boost::get<Parameters::behaviour_removePRGHHydrostaticPressure_type>(
                               &p().behaviour) )
                {
                    if (wt->pressure==Parameters::behaviour_removePRGHHydrostaticPressure_type::totalPressure)
                    {
                        BC["type"]=OFDictData::data ( "prghTotalPressure" );
                        BC["p0"]=OFDictData::data ( "uniform "+boost::lexical_cast<std::string> ( wt->pressure ) );
                    }
                    else if (wt->pressureType==Parameters::behaviour_removePRGHHydrostaticPressure_type::staticPressure)
                    {
                        BC["type"]=OFDictData::data ( "prghPressure" );
                        BC["p"]=OFDictData::data ( "uniform "+boost::lexical_cast<std::string> ( wt->pressure ) );
                    }
                }
                else if ( const auto* extrapol =
                           boost::get<Parameters::behaviour_extrapolate_type>(
                               &p().behaviour) )
                {
                    BC["type"]=OFDictData::data ( "zeroGradient" );
                }
              }

        } else if ( ( field.first=="rho" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]=OFDictData::data (
                "uniform "
                +boost::lexical_cast<std::string> ( p().rho ) );
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



} // namespace insight
