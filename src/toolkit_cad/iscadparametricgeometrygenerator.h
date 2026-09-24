#ifndef ISCADPARAMETRICGEOMETRYGENERATOR_H
#define ISCADPARAMETRICGEOMETRYGENERATOR_H

#include <map>

#include "base/boost_include.h"
#include "base/rapidxml.h"
#include "base/parameterset.h"

#include "cadtypes.h"

namespace insight {


/**
 * @brief The ISCADParametricGeometryGenerator class
 * implements a
 */
class ISCADParametricGeometryGenerator
{
    boost::filesystem::path modelFile_;
    std::map<std::string,double> inputs_;

public:
    declareType("ISCADParametricGeometryGenerator");

    ISCADParametricGeometryGenerator(rapidxml::xml_node<>& node);

    std::unique_ptr<ParameterSet> defaultParameters() const;
    cad::FeaturePtr generateInstance(const ParameterSet& ps) const;
};

} // namespace insight

#endif // ISCADPARAMETRICGEOMETRYGENERATOR_H
