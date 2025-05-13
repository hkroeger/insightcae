#include "compressiblesinglephasethermophysicalproperties.h"

#include "openfoam/caseelements/numerics/fvnumerics.h"
#include "openfoam/caseelements/numerics/buoyantsimplefoamnumerics.h"
#include "openfoam/caseelements/numerics/buoyantpimplefoamnumerics.h"
#include "openfoam/caseelements/numerics/steadycompressiblenumerics.h"
#include "openfoam/caseelements/numerics/unsteadycompressiblenumerics.h"
#include "openfoam/caseelements/numerics/chtmultiregionnumerics.h"
#include <memory>

namespace insight {




defineType(compressibleSinglePhaseThermophysicalProperties);
addToOpenFOAMCaseElementFactoryTable(compressibleSinglePhaseThermophysicalProperties);




void compressibleSinglePhaseThermophysicalProperties::modifyDefaults(ParameterSet &ps)
{
    //auto& specie = ps.get<SelectionParameter>("composition/singleSpecie/properties/fromLibrary/specie");
    auto& specie = ps.get<SelectionParameterInterface>("composition/properties/specie");
    specie.setSelection("N2");
}

compressibleSinglePhaseThermophysicalProperties::compressibleSinglePhaseThermophysicalProperties(
    OpenFOAMCase& c, ParameterSetInput ip )
    : thermodynamicModel(c, ip.forward<Parameters>())
{}




std::string compressibleSinglePhaseThermophysicalProperties::requiredThermoType() const
{
    std::string tt = "hPsiThermo";

    const auto& nce =
        OFcase().findUniqueElement<FVNumerics>();

    if (
        dynamic_cast<const buoyantSimpleFoamNumerics*>(&nce) ||
        dynamic_cast<const buoyantPimpleFoamNumerics*>(&nce) ||
        dynamic_cast<const chtMultiRegionFluidNumerics*>(&nce)
        )
    {
        if (OFversion()<170)
        {
            tt="hPsiThermo";
        }
        else
        {
            tt="heRhoThermo";
        }
    }
    else if (dynamic_cast<const steadyCompressibleNumerics*>(&nce))
    {
        if (OFversion()<170)
        {
            tt="hPsiThermo";
        }
        else
        {
            tt="heRhoThermo";
        }
    }
    else if (const auto t = dynamic_cast<const unsteadyCompressibleNumerics*>(&nce) )
    {
        if (OFversion()<170)
        {
            if (t->p().formulation ==
                unsteadyCompressibleNumerics::Parameters::sonicFoam)
            {
                tt="ePsiThermo";
            }
            else // rhoPimpleFoam
            {
                tt="hPsiThermo";
            }
        }
        else
        {
            tt="hePsiThermo";
        }
    }

    return tt;
}


std::unique_ptr<SpeciesData>
compressibleSinglePhaseThermophysicalProperties::speciesData() const
{
    if (const auto *ss =
        boost::get<Parameters::composition_singleSpecie_type>(&p().composition))
    {
        return std::make_unique<SpeciesData>(*ss);
    }
    else if (const auto *mixdesc=
             boost::get<Parameters::composition_staticSpeciesMixture_type>(&p().composition))
    {
        SpeciesData::SpeciesMixture mix;
        double Mq=0.;

        for (auto& part: mixdesc->components)
        {
            double w;
            SpeciesData sd(part.specie);

            if (const auto* wd =
                boost::get<Parameters::composition_staticSpeciesMixture_type::components_default_type::fraction_massFraction_type>(
                    &part.fraction))
            {
                w = wd->value;
            }
            else if (const auto* mf =
                     boost::get<Parameters::composition_staticSpeciesMixture_type::components_default_type::fraction_moleFraction_type>(
                         &part.fraction))
            {
                double
                    xi = mf->value,
                    Mi = sd.M();

                Mq += xi*Mi;
                w = xi*Mi;
            }
            else throw UnhandledSelection();

            mix.push_back({w, sd});
        }

        for (auto part: boost::adaptors::index(mixdesc->components))
        {
            if (boost::get<Parameters::composition_staticSpeciesMixture_type::components_default_type::fraction_moleFraction_type>(
                    &part.value().fraction))
            {
                mix[part.index()].first /= Mq;
            }
        }

        return std::make_unique<SpeciesData>(mix);
    }
    else throw UnhandledSelection();

    return nullptr;
}


void compressibleSinglePhaseThermophysicalProperties::addIntoDictionaries(OFdicts& dictionaries) const
{

    OFDictData::dict& thermophysicalProperties =
        dictionaries.lookupDict("constant/thermophysicalProperties");

    auto sd = speciesData();

    if (OFversion()<200)
    {
        throw insight::Exception("Unsupported OpenFOAM version!");
    }
    // if (OFversion()<170)
    // {
        // std::string tt = requiredThermoType();

        // tt += "<pureMixture<";

        // std::string mixp_eqn, mixp_thermo, mixp_transp, mixp =
        //                                                 boost::str(boost::format("specie 1 %g") % p_.M);

        // if (const auto *ct = boost::get<Parameters::transport_constant_type>(&p_.transport))
        // {
        //     tt+="constTransport";
        //     mixp_transp =
        //         boost::str(boost::format("%g %g") % ct->mu % ct->Pr );
        // }
        // else if (const auto *st = boost::get<Parameters::transport_sutherland_type>(&p_.transport))
        // {
        //     tt+="sutherlandTransport";
        //     mixp_transp =
        //         boost::str(boost::format("%g %g") % sutherland_As(st->mu, st->Tref) % st->Tref );
        // }

        // tt+="<specieThermo<";

        // if (const auto *ct = boost::get<Parameters::thermo_constant_type>(&p_.thermo))
        // {
        //     tt+="hConstThermo";
        //     mixp_thermo =
        //         boost::str(boost::format("%g %g") % ct->Cp % ct->Hf );
        // }
        // else if (const auto *jt = boost::get<Parameters::thermo_janaf_type>(&p_.thermo))
        // {
        //     tt+="janafThermo";
        //     mixp_thermo =
        //         boost::str(boost::format("%g %g %g\n") % jt->Tlow % jt->Thi % jt->Tmid );

        //     for (arma::uword i=0; i<jt->coeffs_hi.size(); i++)
        //     {
        //         mixp_thermo += " "+boost::lexical_cast<std::string>(jt->coeffs_hi(i));
        //     }
        //     mixp_thermo+="\n";

        //     for (arma::uword i=0; i<jt->coeffs_lo.size(); i++)
        //     {
        //         mixp_thermo += " "+boost::lexical_cast<std::string>(jt->coeffs_lo(i));
        //     }
        //     mixp_thermo+="\n";
        // }

        // tt+="<";

        // if (const auto *pe = boost::get<Parameters::equationOfState_idealGas_type>(&p_.equationOfState))
        // {
        //     tt+="perfectGas";
        // }
        // else if (const auto *pre = boost::get<Parameters::equationOfState_PengRobinson_type>(&p_.equationOfState))
        // {
        //     tt+="PengRobinsonGas";
        //     mixp_eqn =
        //         boost::str(boost::format("%g %g %g %g")
        //                    % pre->Tc % 0.0 % pre->Pc % pre->omega );
        // }
        // else
        // {
        //     throw insight::Exception("Unsupported equation of state!");
        // }

        // tt+=">>>>>";

        // thermophysicalProperties["thermoType"]=tt;
        // thermophysicalProperties["mixture"]=mixp +"\n "+ mixp_eqn +"\n "+ mixp_thermo +"\n "+ mixp_transp;
    // }
    else
    {

        OFDictData::dict thermoType;
        thermoType["type"]=requiredThermoType();
        thermoType["specie"]="specie";
        thermoType["energy"]="sensibleEnthalpy";
        thermoType["mixture"]="pureMixture";
        thermoType["transport"]=sd->transportType();
        thermoType["thermo"]=sd->thermoType();
        thermoType["equationOfState"]=sd->equationOfStateType();
        thermophysicalProperties["thermoType"]=thermoType;

        OFDictData::dict mixdict;
        sd->insertSpecieEntries(mixdict);
        sd->insertThermodynamicsEntries(mixdict);
        sd->insertTransportEntries(mixdict);
        sd->insertEquationOfStateEntries(mixdict);

        thermophysicalProperties["mixture"]=mixdict;
    }

    if (p().requiresAdditionalThermosLib)
    {
        OFDictData::dict& controlDict = dictionaries.lookupDict("system/controlDict");
        controlDict.getList("libs").insertNoDuplicate(
            OFDictData::data("\"libadditionalThermos.so\"") );
    }

}



} // namespace insight
