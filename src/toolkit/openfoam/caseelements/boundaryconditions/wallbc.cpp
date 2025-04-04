#include "wallbc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/turbulencemodel.h"

namespace insight {

defineType(WallBC);
addToFactoryTable(BoundaryCondition, WallBC);
addToStaticFunctionTable(BoundaryCondition, WallBC, defaultParameters);


WallBC::WallBC(
    OpenFOAMCase& c, const std::string& patchName,
    const OFDictData::dict& boundaryDict,
    ParameterSetInput ip)
: BoundaryCondition(
          c, patchName,
          boundaryDict,
          ip.forward<Parameters>())
{
  BCtype_="wall";
}


void WallBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  p().meshmotion->addIntoDictionaries(dictionaries);
  BoundaryCondition::addIntoDictionaries(dictionaries);
}

void WallBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
    multiphaseBC::multiphaseBCPtr phasefractions =
        p().phasefractions;

    HeatBC::HeatBCPtr heattransfer =
        p().heattransfer;

    MeshMotionBC::MeshMotionBCPtr meshmotion =
        p().meshmotion;

    BoundaryCondition::addIntoFieldDictionaries(dictionaries);

    for (const FieldList::value_type& field: OFcase().fields())
    {
        OFDictData::dict& BC = dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
                               .subDict("boundaryField").subDict(patchName_);

        // velocity
        if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
        {
            if (p().rotating)
            {
                BC["type"]=OFDictData::data("rotatingWallVelocity");
                BC["origin"]=OFDictData::to_OF(p().CofR);
                double om=norm(p().wallVelocity, 2);
                BC["axis"]=OFDictData::to_OF(p().wallVelocity/om);
                BC["omega"]=boost::lexical_cast<std::string>(om);
            }
            else
            {
                BC["type"]=OFDictData::data("movingWallVelocity");
                BC["value"]=OFDictData::data("uniform "+OFDictData::to_OF(p().wallVelocity));
            }
        }

        // pressure
        else if ( (field.first=="p") && (get<0>(field.second)==scalarField) )
        {
            BC["type"]=OFDictData::data("zeroGradient");
        }

        // pressure
        else if ( isPrghPressureField(field) )
        {
            if (OFversion()>=220)
                BC["type"]=OFDictData::data("fixedFluxPressure");
            else
                BC["type"]=OFDictData::data("buoyantPressure");
        }

        // turbulence quantities, should be handled by turbulence model
        else if (
            ( (field.first=="k") || (field.first=="omega") || (field.first=="epsilon") ||
              (field.first=="nut") || (field.first=="nuSgs") || (field.first=="nuTilda") ||
              (field.first=="alphat") )
            &&
            (get<0>(field.second)==scalarField)
        )
        {
            OFcase().findUniqueElement<turbulenceModel>()
                .addIntoFieldDictionary(
                    field.first, field.second, BC, p().roughness_z0 );
        }

        else
        {
            bool handled = false;

            handled = meshmotion->addIntoFieldDictionary(field.first, field.second, BC) || handled;
            handled = phasefractions->addIntoFieldDictionary ( field.first, field.second, BC ) || handled;
            handled = heattransfer->addIntoFieldDictionary ( field.first, field.second, BC, dictionaries ) || handled;

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

  HeatBC::HeatBCPtr heattransfer =
      p().heattransfer;
  heattransfer->addOptionsToBoundaryDict(bndDict);
}

} // namespace insight
