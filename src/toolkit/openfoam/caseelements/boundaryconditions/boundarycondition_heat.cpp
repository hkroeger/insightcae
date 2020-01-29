
#include "openfoam/caseelements/boundaryconditions/boundarycondition_heat.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;

namespace insight
{
namespace HeatBC
{

defineType(HeatBC);
defineDynamicClass(HeatBC);

HeatBC::~HeatBC()
{
}


void HeatBC::addIntoDictionaries(OFdicts&) const
{}




defineType(AdiabaticBC);
addToFactoryTable(HeatBC, AdiabaticBC);
addToStaticFunctionTable(HeatBC, AdiabaticBC, defaultParameters);

AdiabaticBC::AdiabaticBC(const ParameterSet& ps)
{}

bool AdiabaticBC::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, OFdicts&) const
{
    if
    (
      (fieldname=="T")
      &&
      (get<0>(fieldinfo)==scalarField)
    )
    {
      BC["type"]="zeroGradient";
      return true;
    }
    else
      return false;
}





defineType(FixedTemperatureBC);
addToFactoryTable(HeatBC, FixedTemperatureBC);
addToStaticFunctionTable(HeatBC, FixedTemperatureBC, defaultParameters);

FixedTemperatureBC::FixedTemperatureBC(const ParameterSet& ps)
: p_(ps)
{}

bool FixedTemperatureBC::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, OFdicts& dictionaries) const
{
    if
    (
      (fieldname=="T")
      &&
      (get<0>(fieldinfo)==scalarField)
    )
    {
      FieldData(p_.T).setDirichletBC(BC, dictionaries);
      return true;
    }
    else
      return false;
}





defineType(ExternalWallBC);
addToFactoryTable(HeatBC, ExternalWallBC);
addToStaticFunctionTable(HeatBC, ExternalWallBC, defaultParameters);

ExternalWallBC::ExternalWallBC(const ParameterSet& ps)
: p_(ps)
{}

bool ExternalWallBC::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, OFdicts&) const
{
    if
    (
      (fieldname=="T")
      &&
      (get<0>(fieldinfo)==scalarField)
    )
    {
        BC["type"]="externalWallHeatFluxTemperature";

        if (const auto* ft = boost::get<Parameters::kappaSource_fluidThermo_type>(&p_.kappaSource))
        {
          BC["kappaMethod"]="fluidThermo";
          BC["kappa"]="none";
        }
        else if (const auto* lu = boost::get<Parameters::kappaSource_lookup_type>(&p_.kappaSource))
        {
          BC["kappaMethod"]="lookup";
          BC["kappa"]=lu->kappaFieldName;
        }

        BC["Qr"]="none";
        BC["relaxation"]=1.0;

        if (const auto* const_p = boost::get<Parameters::heatflux_fixedPower_type>(&p_.heatflux))
          {
            BC["mode"]="power";
            BC["Q"]=boost::str(boost::format("%g")%const_p->Q );
          }
        else if (const auto* const_q = boost::get<Parameters::heatflux_fixedHeatFlux_type>(&p_.heatflux))
          {
            BC["mode"]="flux";
            BC["q"]=boost::str(boost::format("uniform %g")%const_q->q );
          }
        else if (const auto* conv_q = boost::get<Parameters::heatflux_fixedHeatTransferCoeff_type>(&p_.heatflux))
          {
            BC["mode"]="coefficient";
            BC["Ta"]=boost::str(boost::format("uniform %g") % conv_q->Ta );
            BC["h"]=boost::str(boost::format("uniform %g") % conv_q->h );
          }

        OFDictData::list tL, kL;
        for (const Parameters::wallLayers_default_type& layer: p_.wallLayers)
        {
          tL.push_back(layer.thickness);
          kL.push_back(layer.kappa);
        }
        BC["thicknessLayers"]=tL;
        BC["kappaLayers"]=kL;

        BC["value"]=boost::str(boost::format("uniform %g") % 300.0 );
      return true;
    }
    else
      return false;
}


}
}
