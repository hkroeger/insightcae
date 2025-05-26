#include "materialslibrary.h"
#include "base/rapidxml.h"

namespace insight {


defineType(Material);


Material::Material()
{}


Material::Material(rapidxml::xml_node<> &node)
{
    CurrentExceptionContext ex("reading material data");

    density_ = toNumber<double>(getMandatoryAttribute(node, "density"))
            * si::kilogram_per_cubic_meter;

    youngsModulus_ = toNumber<double>(getMandatoryAttribute(node, "youngsModulus"))
            *1e6*si::pascals;

    poissonNumber_ = toNumber<double>(getMandatoryAttribute(node, "poissonNumber"));

    cp_ = toNumber<double>(getMandatoryAttribute(node, "specificHeatCapacity"))
            * si::joule/si::kilogram/si::kelvin;

    kappa_ = toNumber<double>(getMandatoryAttribute(node, "heatConductivity"))
            * si::watt/si::meter/si::kelvin;
}



} // namespace insight
