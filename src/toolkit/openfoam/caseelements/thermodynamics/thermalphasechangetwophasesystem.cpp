#include "thermalphasechangetwophasesystem.h"
#include "openfoam/caseelements/basic/thermodynamicmodel.h"

#include "openfoam/ofdicts.h"

namespace insight {

namespace thermalPhaseChangeTwoPhaseSystemModels {


defineType(diameterModel);
defineDynamicClass(diameterModel);

diameterModel::diameterModel(ParameterSetInput ip)
    : p_(ip.forward<Parameters>())
{}


defineType(constantDiameter);
addToFactoryTable(diameterModel, constantDiameter);
addToStaticFunctionTable(diameterModel, constantDiameter, defaultParameters);


constantDiameter::constantDiameter(ParameterSetInput ip)
    :diameterModel(ip.forward<Parameters>())
{}

void constantDiameter::addIntoPhaseDict(OFDictData::dict& pd) const
{
    pd.merge_overwrite({
        {"diameterModel", "constant"},
        {"constantCoeffs", OFDictData::dict{
                               {"d", p().d}
                           }}
    });
}


defineType(isothermalDiameter);
addToFactoryTable(diameterModel, isothermalDiameter);
addToStaticFunctionTable(diameterModel, isothermalDiameter, defaultParameters);


isothermalDiameter::isothermalDiameter(ParameterSetInput ip)
    :diameterModel(ip.forward<Parameters>())
{}

void isothermalDiameter::addIntoPhaseDict(OFDictData::dict& pd) const
{
    pd+={
         {"diameterModel", "isothermal"},
         {"isothermalCoeffs", OFDictData::dict{
                                     {"d0", p().d0},
                                     {"p0", p().p0}
                                 }
         }
        };
}




defineType(phaseModel);
defineDynamicClass(phaseModel);

phaseModel::phaseModel(ParameterSetInput ip)
    : p_(ip.forward<Parameters>())
{}

void phaseModel::addIntoPhaseDict(OFDictData::dict &pd) const
{
    pd+={
            {"Sct", p().Sct},
            {"residualAlpha", p().residualAlpha}
        };
}


defineType(purePhase);
addToFactoryTable(phaseModel, purePhase);
addToStaticFunctionTable(phaseModel, purePhase, defaultParameters);


purePhase::purePhase(ParameterSetInput ip)
    :phaseModel(ip.forward<Parameters>())
{}

void purePhase::addIntoPhaseDict(OFDictData::dict& pd) const
{
    phaseModel::addIntoPhaseDict(pd);
}



defineType(dragModel);
defineDynamicClass(dragModel);

dragModel::dragModel(ParameterSetInput ip)
    : p_(ip.forward<Parameters>())
{}




defineType(SchillerNaumannDrag);
addToFactoryTable(dragModel, SchillerNaumannDrag);
addToStaticFunctionTable(dragModel, SchillerNaumannDrag, defaultParameters);

SchillerNaumannDrag::SchillerNaumannDrag(ParameterSetInput ip)
    : dragModel(ip.forward<Parameters>())
{}

OFDictData::dict SchillerNaumannDrag::dragModelDict() const
{
    return {
    };
}






defineType(blendingModel);
defineDynamicClass(blendingModel);

blendingModel::blendingModel(ParameterSetInput ip)
    : p_(ip.forward<Parameters>())
{}


defineType(noBlending);
addToFactoryTable(blendingModel, noBlending);
addToStaticFunctionTable(blendingModel, noBlending, defaultParameters);

noBlending::noBlending(ParameterSetInput ip)
    : blendingModel(ip.forward<Parameters>())
{}

OFDictData::dict noBlending::blendingProperties() const
{
    return {
        { "type", "none" },
        { "continuousPhase", p().continuousPhase }
    };
}


defineType(saturationModel);
defineDynamicClass(saturationModel);

saturationModel::saturationModel(ParameterSetInput ip)
    : p_(ip.forward<Parameters>())
{}



defineType(TabulatedSaturation);
addToFactoryTable(saturationModel, TabulatedSaturation);
addToStaticFunctionTable(saturationModel, TabulatedSaturation, defaultParameters);

TabulatedSaturation::TabulatedSaturation(ParameterSetInput ip)
    : saturationModel(ip.forward<Parameters>())
{}

OFDictData::dict TabulatedSaturation::saturationModelDict() const
{
    return {
    };
}



defineType(heatTransferModel);
defineDynamicClass(heatTransferModel);

heatTransferModel::heatTransferModel(ParameterSetInput ip)
    : p_(ip.forward<Parameters>())
{}


defineType(sphericalHeatTransfer);
addToFactoryTable(heatTransferModel, sphericalHeatTransfer);
addToStaticFunctionTable(heatTransferModel, sphericalHeatTransfer, defaultParameters);

sphericalHeatTransfer::sphericalHeatTransfer(ParameterSetInput ip)
    : heatTransferModel(ip.forward<Parameters>())
{}

OFDictData::dict sphericalHeatTransfer::heatTransferModelDict() const
{
    return {
    };
}



defineType(turbulentDispersionModel);
defineDynamicClass(turbulentDispersionModel);

turbulentDispersionModel::turbulentDispersionModel(ParameterSetInput ip)
    : p_(ip.forward<Parameters>())
{}


defineType(constantCoefficient);
addToFactoryTable(turbulentDispersionModel, constantCoefficient);
addToStaticFunctionTable(turbulentDispersionModel, constantCoefficient, defaultParameters);

constantCoefficient::constantCoefficient(ParameterSetInput ip)
    : turbulentDispersionModel(ip.forward<Parameters>())
{}

OFDictData::dict constantCoefficient::turbulentDispersionModelDict() const
{
    return {
    };
}

}




defineType(thermalPhaseChangeTwoPhaseSystem);
addToOpenFOAMCaseElementFactoryTable(thermalPhaseChangeTwoPhaseSystem);

thermalPhaseChangeTwoPhaseSystem::thermalPhaseChangeTwoPhaseSystem(OpenFOAMCase &c, ParameterSetInput ip)
    : thermodynamicModel(c, ip.forward<Parameters>())
{}

void thermalPhaseChangeTwoPhaseSystem::addIntoDictionaries(OFdicts &dictionaries) const
{
    OFDictData::dict& pp=dictionaries.lookupDict("constant/phaseProperties");

    pp+={
        {"type", "thermalPhaseChangeTwoPhaseSystem"},
        {"phaseChange", p().phaseChange},
        {"pMin", p().pMin}
    };

    OFDictData::list phaseList;
    for (auto &phase: p().phases)
    {
        phaseList.push_back(phase.first);
        OFDictData::dict phaseDict;
        phase.second->addIntoPhaseDict(phaseDict);
        pp[phase.first]=phaseDict;
    }

    {
        OFDictData::dict bd;
        for (auto&bl: p().blending)
        {
            bd[bl.first]=bl.second->blendingProperties();
        }
        pp["blending"]=bd;
    }

    pp["surfaceTension"]=OFDictData::list();

    pp["saturationModel"]=p().saturationModel->saturationModelDict();

    pp["aspectRatio"]=OFDictData::list();

    {
        OFDictData::list dml;
        for (auto &dm: p().drag)
        {
            dml.push_back(str(boost::format(
                          "(%s in %s)"
                          ) % dm.first % dm.second.inPhase
                      ));
            dml.push_back(
                dm.second.model->dragModelDict()
                );
        }
        pp["drag"]=dml;
    }

    {
        std::map<std::string, OFDictData::list> htmls;
        for (auto &htm: p().heatTransfer)
        {
            auto &html=htmls[htm.first];
            html.push_back(str(boost::format(
                                   "(%s in %s)"
                                   ) % htm.second.inPhase % htm.first
                               ));
            html.push_back(
                htm.second.model->heatTransferModelDict()
                );
        }
        for (auto&html: htmls)
        {
            pp["heatTransfer."+html.first]=html.second;
        }
    }

    pp["phaseTransfer"]=OFDictData::list();

    pp["lift"]=OFDictData::list();

    pp["wallLubrication"]=OFDictData::list();

    {
        OFDictData::list tdml;
        for (auto &dm: p().turbulentDispersion)
        {
            tdml.push_back(str(boost::format(
                                  "(%s in %s)"
                                  ) % dm.first % dm.second.inPhase
                              ));
            tdml.push_back(
                dm.second.model->turbulentDispersionModelDict()
                );
        }
        pp["turbulentDispersion"]=tdml;
    }
}


bool thermalPhaseChangeTwoPhaseSystem::isUnique() const
{
    return false;
}

} // namespace insight
