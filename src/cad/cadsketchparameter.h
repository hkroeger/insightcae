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


/**
 * @brief The DelayedCreatedSketch class
 * keeps a script and converts it into a sketch when needed first
 * (i.e. when sketchRef() is called)
 */
class DelayedCreatedSketch
{
    mutable std::unique_ptr<std::string> script_;
    std::shared_ptr<insight::cad::ConstrainedSketch> sketch_;

    virtual void connectSignalsToSketch(insight::cad::ConstrainedSketchPtr sketch) =0;
    virtual cad::ConstrainedSketchPtr createSketch(const std::string& script) const =0;

public:
    virtual cad::ConstrainedSketchPtr createEmpty() const =0;

    insight::cad::ConstrainedSketch& sketchRef();

    inline const insight::cad::ConstrainedSketch& sketch() const
    { return const_cast<DelayedCreatedSketch&>(*this).sketchRef(); }

    inline cad::FeaturePtr featureGeometry() const
    { return const_cast<DelayedCreatedSketch&>(*this).featureGeometry(); }

    inline cad::FeaturePtr featureGeometryRef() const
    {
        sketch(); // trigger creation
        return sketch_;
    }

    void setScript(const std::string& script);
    std::string script() const;

    void assignFrom(const DelayedCreatedSketch& dcs);
};



class CADSketchParameter
: public CADGeometryParameter,
  public DelayedCreatedSketch
{

public:
    typedef std::map<int, std::string> References;

protected:
    std::shared_ptr<insight::cad::ConstrainedSketchParametersDelegate> entityProperties_;
    std::string presentationDelegateKey_;

    std::map<int, std::string> references_;

    boost::signals2::scoped_connection
        addSlotConn_, removeSlotConn_, changeSlotConn_;


    void connectSignalsToSketch(insight::cad::ConstrainedSketchPtr sketch) override;
    cad::ConstrainedSketchPtr createSketch(const std::string& script) const override;

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

    std::shared_ptr<insight::cad::ConstrainedSketch> createEmpty() const override;

    void setReferences(const std::map<int, std::string>& references);

    std::shared_ptr<insight::cad::ConstrainedSketchParametersDelegate> entityProperties() const;
    const std::string& presentationDelegateKey() const;

    void setScript(const std::string& script);

    cad::FeaturePtr geometry() const override;

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

    std::unique_ptr<hierarchicalData::Element> clone() const override;

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
