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
        if ( (field.first=="U") && (boost::fusion::get<0>(field.second)==vectorField) )
        {
            if (p().rotating)
            {
                BC["type"]=OFDictData::data("rotatingWallVelocity");
                BC["origin"]=OFDictData::vector3(p().CofR);
                double om=norm(p().wallVelocity, 2);
                BC["axis"]=OFDictData::vector3(p().wallVelocity/om);
                BC["omega"]=toString(om);
            }
            else
            {
                BC["type"]=OFDictData::data("movingWallVelocity");
                BC["value"]=OFDictData::toUniformField(p().wallVelocity);
            }
        }

        // pressure
        else if ( (field.first=="p") && (boost::fusion::get<0>(field.second)==scalarField) )
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

        else
        {
            bool handled = false;

            // turbulence quantities: try each turbulence model element in the case
            // (supports multiple phases via phaseName parameter on each model)
            for (auto* tm : OFcase().findElements<turbulenceModel>())
            {
                if (tm->addIntoFieldDictionary(field.first, field.second, BC, p().roughness_z0))
                {
                    handled = true;
                    break;
                }
            }

            if (!handled)
            {
                handled = handled || meshmotion->addIntoFieldDictionary(field.first, field.second, BC);
                handled = handled || phasefractions->addIntoFieldDictionary ( field.first, field.second, BC );
                handled = handled || heattransfer->addIntoFieldDictionary ( field.first, field.second, BC, dictionaries );
            }

            if (!handled)
            {
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
