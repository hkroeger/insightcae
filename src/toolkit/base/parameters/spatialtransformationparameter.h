#ifndef INSIGHT_SPATIALTRANSFORMATIONPARAMETER_H
#define INSIGHT_SPATIALTRANSFORMATIONPARAMETER_H

#include "base/parameter.h"
#include "base/spatialtransformation.h"

namespace insight {

class SpatialTransformationParameter
        : public Parameter,
          public SpatialTransformation
{
public:
    declareType ( "spatialTransformation" );
    SpatialTransformationParameter(
            const std::string& description,
            bool isHidden=false,
            bool isExpert=false,
            bool isNecessary=false,
            int order=0 );

    SpatialTransformationParameter(
            const SpatialTransformation& trsf,
            const std::string& description,
            bool isHidden=false,
            bool isExpert=false,
            bool isNecessary=false,
            int order=0 );

    bool isDifferent(const Parameter& p) const override;

    SpatialTransformation& operator() ()
    {
        return *this;
    }

    const SpatialTransformation& operator() () const
    {
        return *this;
    }

    std::string latexRepresentation() const override;
    std::string plainTextRepresentation(int indent=0) const override;

    rapidxml::xml_node<>* appendToNode (
            const std::string& name,
            rapidxml::xml_document<>& doc,
            rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) const override;

    void readFromNode (
            const std::string& name,
            rapidxml::xml_document<>& doc,
            rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) override;

    Parameter* clone() const override;
    void reset(const Parameter& p) override;
};

} // namespace insight

#endif // INSIGHT_GEOMETRYTRANSFORMATIONPARAMETER_H
