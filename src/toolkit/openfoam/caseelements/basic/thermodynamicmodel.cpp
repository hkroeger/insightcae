#include "thermodynamicmodel.h"

namespace insight
{



thermodynamicModel::thermodynamicModel(
    OpenFOAMCase& c, ParameterSetInput ip)
: OpenFOAMCaseElement(c, ip.forward<Parameters>())
{}


std::string thermodynamicModel::category()
{
    return "Thermodynamics";
}



thermophysicalModel::thermophysicalModel(OpenFOAMCase &c, ParameterSetInput ip)
    : thermodynamicModel(c, ip.forward<Parameters>())
{}

std::string thermophysicalModel::thermophysicalPropertiesDictName() const
{
    const std::string& phase = p().phaseName;
    if (phase.empty()) return "constant/thermophysicalProperties";
    return "constant/thermophysicalProperties." + phase;
}

}
