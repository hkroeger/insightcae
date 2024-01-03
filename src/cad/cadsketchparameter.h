#ifndef INSIGHT_CADSKETCHPARAMETER_H
#define INSIGHT_CADSKETCHPARAMETER_H

#include "cadtypes.h"
#include "base/parameter.h"
#include "base/parameterset.h"

#include "cadgeometryparameter.h"
#include "constrainedsketchgeometry.h"

namespace insight {

namespace cad {
class ConstrainedSketch;
}

class CADSketchParameter
: public CADGeometryParameter
{


protected:
    //    insight::cad::ModelPtr cadmodel_;

    cad::MakeDefaultGeometryParametersFunction makeDefaultGeometryParameters;
    std::map<int, std::string> references_;

    mutable std::unique_ptr<std::string> script_;
    mutable std::shared_ptr<insight::cad::ConstrainedSketch> CADGeometry_;

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
        cad::MakeDefaultGeometryParametersFunction defaultGeometryParameters,
        const std::map<int, std::string>& references,
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );


    void setReferences(const std::map<int, std::string>& references);

    insight::ParameterSet defaultGeometryParameters() const;

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
            rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath
            ) override;

    CADSketchParameter* cloneCADSketchParameter(bool keepParentRef=false) const;
    Parameter* clone() const override;

    void copyFrom(const Parameter& op) override;
    void operator=(const CADSketchParameter& op);

    bool isDifferent(const Parameter &) const override;

    int nChildren() const override;
    std::string childParameterName( int i ) const override;
    Parameter& childParameterRef ( int i ) override;
    const Parameter& childParameter( int i ) const override;

};

} // namespace insight

#endif // INSIGHT_CADSKETCHPARAMETER_H
