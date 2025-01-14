#ifndef INSIGHT_SPATIALTRANSFORMATIONPARAMETER_H
#define INSIGHT_SPATIALTRANSFORMATIONPARAMETER_H

#include "base/parameter.h"
#include "base/spatialtransformation.h"

namespace insight {

class SpatialTransformationParameter
        : public Parameter,
          private SpatialTransformation
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

    void set(const SpatialTransformation& nv)
    {
        SpatialTransformation::operator=(nv);
        triggerValueChanged();
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
            rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) override;

    std::unique_ptr<Parameter> clone() const override;
    void copyFrom(const Parameter& p) override;
    void operator=(const SpatialTransformationParameter& p);

    int nChildren() const override;
};

} // namespace insight

#endif // INSIGHT_GEOMETRYTRANSFORMATIONPARAMETER_H
