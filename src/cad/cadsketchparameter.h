#ifndef INSIGHT_CADSKETCHPARAMETER_H
#define INSIGHT_CADSKETCHPARAMETER_H

#include "boost/signals2/connection.hpp"
#include "cadtypes.h"
#include "base/parameter.h"
#include "base/parameterset.h"

#include "cadgeometryparameter.h"
#include "constrainedsketch.h"
#include "constrainedsketchentity.h"

namespace insight {

namespace cad {
class ConstrainedSketch;
}

class CADSketchParameter
: public CADGeometryParameter
{

public:
    typedef std::map<int, std::string> References;

protected:
    std::shared_ptr<insight::cad::ConstrainedSketchParametersDelegate> entityProperties_;
    std::string presentationDelegateKey_;

    std::map<int, std::string> references_;

    mutable std::unique_ptr<std::string> script_;
    mutable std::shared_ptr<insight::cad::ConstrainedSketch> CADGeometry_;

    boost::signals2::scoped_connection
        addSlotConn_, removeSlotConn_, changeSlotConn_;

    void regenerateScript();
    void resetCADGeometry();

public:
    declareType ( "cadsketch" );

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


    void setReferences(const std::map<int, std::string>& references);

    std::shared_ptr<insight::cad::ConstrainedSketchParametersDelegate> entityProperties() const;
    const std::string& presentationDelegateKey() const;

    std::string script() const;
    void setScript(const std::string& script);

    const insight::cad::ConstrainedSketch& featureGeometry() const;
    std::shared_ptr<insight::cad::ConstrainedSketch> featureGeometryRef();

    cad::FeaturePtr geometry() const override;

    std::string latexRepresentation() const override;
    std::string plainTextRepresentation(int indent=0) const override;

    rapidxml::xml_node<>* appendToNode
        (
            const std::string& name,
            rapidxml::xml_document<>& doc,
            rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath
            ) const override;

    void readFromNode
        (
            const std::string& name,
            const rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath
        ) override;

    std::unique_ptr<CADSketchParameter>
        cloneCADSketchParameter(
            bool keepParentRef=false ) const;
    std::unique_ptr<Parameter> clone(bool initialize) const override;

    void copyFrom(const Parameter& op) override;
    void operator=(const CADSketchParameter& op);

    bool isDifferent(const Parameter &) const override;

    int nChildren() const override;
    std::string childParameterName(
        int i,
        bool redirectArrayElementsToDefault=false ) const override;
    Parameter& childParameterRef ( int i ) override;
    const Parameter& childParameter( int i ) const override;

};

} // namespace insight

#endif // INSIGHT_CADSKETCHPARAMETER_H
