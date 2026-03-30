#ifndef INSIGHT_CADSKETCHPARAMETER_H
#define INSIGHT_CADSKETCHPARAMETER_H

#include "base/hierarchicalelement.h"
#include "boost/signals2/connection.hpp"
#include "cadtypes.h"
#include "base/parameter.h"
#include "base/parameterset.h"

#include "cadgeometryparameter.h"
#include "constrainedsketch.h"
#include "constrainedsketchentity.h"
#include <memory>




namespace insight {



namespace cad {
class ConstrainedSketch;
}




class CADSketchParameter
: public CADGeometryParameterBase
{

public:
    typedef std::map<int, std::string> References;

protected:
    std::shared_ptr<insight::cad::ConstrainedSketchParametersDelegate> entityProperties_; ///< needs to be first

    mutable std::unique_ptr<std::string> initialScript_;
    std::shared_ptr<insight::cad::ConstrainedSketch> sketch_;

    std::string presentationDelegateKey_;

    std::map<int, std::string> references_;

    boost::signals2::scoped_connection
        befAddSlotConn_, addSlotConn_,
        befRemoveSlotConn_, removeSlotConn_,
        changeSlotConn_;

    boost::signals2::scoped_connection
        befAddLayerSlotConn_, addLayerSlotConn_,
        befRemoveLayerSlotConn_, removeLayerSlotConn_,
        changeLayerSlotConn_;


    void connectSignalsToSketch(insight::cad::ConstrainedSketchPtr sketch);
    void afterInPlaceSketchUpdate(insight::cad::ConstrainedSketchPtr sketch);
    cad::ConstrainedSketchPtr createSketch(const std::string& script) const;

public:
    declareType ( "cadsketch" );

    CADSketchParameter (
        const rapidxml::xml_node<>& node );

    CADSketchParameter (
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0  );

    CADSketchParameter (
        const std::string& script,
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );

    CADSketchParameter (
        const std::string& script,
        std::shared_ptr<insight::cad::ConstrainedSketchParametersDelegate> entityProperties,
        const std::string& presentationDelegateKey,
        const References& references,
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );

    ~CADSketchParameter();

    void initializeHierarchy() override;

    std::shared_ptr<insight::cad::ConstrainedSketch> createEmpty() const;

    bool hasReferences() const;
    // void setReferences(const std::map<int, std::string>& references);

    std::shared_ptr<insight::cad::ConstrainedSketchParametersDelegate> entityProperties() const;
    const std::string& presentationDelegateKey() const;

    void setScript(const std::string& script);
    std::string script() const;

    cad::FeaturePtr geometry() const override;

    const insight::cad::ConstrainedSketch& sketch() const;
    insight::cad::ConstrainedSketch& sketchRef();

    cad::ConstFeaturePtr featureGeometry() const;
    cad::FeaturePtr featureGeometryRef();

    std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const override;

    std::string plainTextRepresentation(int indent) const override;

    rapidxml::xml_node<>* appendToNode
        (
            const std::string& name,
            rapidxml::xml_document<>& doc,
            rapidxml::xml_node<>& node,
            const OutputProperties& outProps
            ) const override;

    const rapidxml::xml_node<>* readFromNode
        (
            const std::string& name,
            const rapidxml::xml_node<>& node
        ) override;

protected:
    std::unique_ptr<hierarchicalData::Element> cloneUninitialized() const override;

public:
    void assignFrom( const Element& rhs ) override;
    void copyMatching( const Element& rhs ) override;
    void extend( const Element& op ) override;
    bool isEqual(const Element& op) const override;

    bool isDifferent(const Parameter &) const override;

    int nChildren() const override;
    std::string childElementName(
        int i,
        bool redirectArrayElementsToDefault=false ) const override;
    Parameter& childElementRef ( int i ) override;
    const Parameter& childElement( int i ) const override;

};

} // namespace insight

#endif // INSIGHT_CADSKETCHPARAMETER_H
