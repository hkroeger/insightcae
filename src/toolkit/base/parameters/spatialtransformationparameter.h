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
        const rapidxml::xml_node<> & node);

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

    std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const override;

    std::string plainTextRepresentation(int indent) const override;

    rapidxml::xml_node<>* appendToNode (
            const std::string& name,
            rapidxml::xml_document<>& doc,
            rapidxml::xml_node<>& node ) const override;

    const rapidxml::xml_node<>* readFromNode (
            const std::string& name,
            const rapidxml::xml_node<>& node ) override;

    std::unique_ptr<Element> clone() const override;
    void assignFrom(const Element& p) override;
    bool isEqual(const Element& op) const override;

    int nChildren() const override;
};

} // namespace insight

#endif // INSIGHT_GEOMETRYTRANSFORMATIONPARAMETER_H
