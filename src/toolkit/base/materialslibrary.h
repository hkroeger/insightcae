#ifndef INSIGHT_MATERIALSLIBRARY_H
#define INSIGHT_MATERIALSLIBRARY_H

#include "base/units.h"
#include "base/propertylibrary.h"
#include "base/factory.h"


namespace insight {

class Material
{
public:
    declareType("material");

    Material();
    Material(rapidxml::xml_node<>& node);

    boost::units::quantity<si::mass_density, double> density_;
    boost::units::quantity<si::pressure, double> youngsModulus_;
    double poissonNumber_;

    boost::units::quantity<si::specific_heat_capacity, double> cp_;
    boost::units::quantity<si::thermal_conductivity, double> kappa_;

};


typedef PropertyLibrary<Material> materials;


} // namespace insight

#endif // INSIGHT_MATERIALSLIBRARY_H
