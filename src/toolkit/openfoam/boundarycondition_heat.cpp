
#include "openfoam/boundarycondition_heat.h"

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

bool AdiabaticBC::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
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

bool FixedTemperatureBC::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
    if
    (
      (fieldname=="T")
      &&
      (get<0>(fieldinfo)==scalarField)
    )
    {
      FieldData(p_.T).setDirichletBC(BC);
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

bool ExternalWallBC::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
    if
    (
      (fieldname=="T")
      &&
      (get<0>(fieldinfo)==scalarField)
    )
    {
        BC["type"]="externalWallHeatFluxTemperature";

        BC["kappaMethod"]="fluidThermo";
        BC["kappa"]="none";

        BC["Qr"]="none";
        BC["relaxation"]=1.0;

        if (const Parameters::heatflux_constant_type* const_q = boost::get<Parameters::heatflux_constant_type>(&p_.heatflux))
          {
            BC["q"]=boost::str(boost::format("uniform %g")%const_q->q );
          }
        else if (const Parameters::heatflux_convective_type* conv_q = boost::get<Parameters::heatflux_convective_type>(&p_.heatflux))
          {
            BC["Ta"]=boost::str(boost::format("uniform %g") % conv_q->Ta );
            BC["h"]=boost::str(boost::format("uniform %g") % conv_q->h );
          }

        OFDictData::list tL, kL;
        BOOST_FOREACH(const Parameters::wallLayers_default_type& layer, p_.wallLayers)
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
