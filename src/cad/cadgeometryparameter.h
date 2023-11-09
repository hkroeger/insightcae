#ifndef INSIGHT_CADGEOMETRYPARAMETER_H
#define INSIGHT_CADGEOMETRYPARAMETER_H

#include "cadtypes.h"
#include "base/parameters/pathparameter.h"

namespace insight {

class CADGeometryParameter
        : public Parameter
{
protected:
//    insight::cad::ModelPtr cadmodel_;
    std::string featureLabel_, script_;

    mutable insight::cad::FeaturePtr CADGeometry_;

    void resetCADGeometry() const;

public:
    declareType ( "cadgeometry" );

    CADGeometryParameter (
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0  );

    CADGeometryParameter (
        const std::string& featureLabel,
        const std::string& script,
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );


    const std::string& featureLabel() const;
    void setFeatureLabel(const std::string& label);

    const std::string& script() const;
    void setScript(const std::string& script);

//    void setCADModel(insight::cad::ModelPtr cadmodel);
    cad::FeaturePtr featureGeometry() const;

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

    CADGeometryParameter* cloneCADGeometryParameter() const;
    Parameter* clone() const override;

    void operator=(const CADGeometryParameter& op);

    bool isDifferent(const Parameter &) const override;

    int nChildren() const override;
};


} // namespace insight

#endif // INSIGHT_CADGEOMETRYPARAMETER_H
