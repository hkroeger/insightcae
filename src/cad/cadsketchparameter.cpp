#include "cadsketchparameter.h"

#include "sketch.h"
#include "datum.h"
#include "base/rapidxml.h"


namespace insight {




defineType(CADSketchParameter);
addToFactoryTable(Parameter, CADSketchParameter);



void CADSketchParameter::resetCADGeometry() const
{
    if (!script().empty())
    {
        std::istringstream is(script());

        CADGeometry_ = cad::ConstrainedSketch::createFromStream(
            std::make_shared<cad::DatumPlane>(
                cad::vec3const(0,0,0),
                cad::vec3const(0,0,1) ),
            is,
            makeDefaultGeometryParameters()
            );
    }
    else
    {
        CADGeometry_.reset();
    }
}

CADSketchParameter::CADSketchParameter(
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order
    )
    : Parameter(description, isHidden, isExpert, isNecessary, order)
{}




CADSketchParameter::CADSketchParameter(
    const std::string& script,
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order
    )
    : Parameter(description, isHidden, isExpert, isNecessary, order),
    script_(script)
{
    resetCADGeometry();
}


CADSketchParameter::CADSketchParameter(
   const std::string& script,
    cad::MakeDefaultGeometryParametersFunction mdpf,
   const std::string& description,
   bool isHidden, bool isExpert, bool isNecessary, int order
   )
    : Parameter(description, isHidden, isExpert, isNecessary, order),
    makeDefaultGeometryParameters(mdpf),
    script_(script)
{
    resetCADGeometry();
}



ParameterSet CADSketchParameter::defaultGeometryParameters() const
{
    return makeDefaultGeometryParameters();
}



const std::string& CADSketchParameter::script() const
{
    return script_;
}


void CADSketchParameter::setScript(const std::string& script)
{
    script_=script;
    resetCADGeometry();
}


std::shared_ptr<insight::cad::ConstrainedSketch>
CADSketchParameter::featureGeometry() const
{
    return CADGeometry_;
}


std::string CADSketchParameter::latexRepresentation() const
{
    return "(sketched)";
}

std::string CADSketchParameter::plainTextRepresentation(int indent) const
{
    return "(sketched)";
}

rapidxml::xml_node<>* CADSketchParameter::appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        boost::filesystem::path inputfilepath
        ) const
{
    auto n = Parameter::appendToNode(name, doc, node, inputfilepath);
    n->value(doc.allocate_string(script_.c_str()));
    return n;
}

void CADSketchParameter::readFromNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        boost::filesystem::path inputfilepath
        )
{
    using namespace rapidxml;
    xml_node<>* child = findNode(node, name, type());
    if (child)
    {
        script_ = child->value();

        resetCADGeometry();
    }
}

CADSketchParameter *CADSketchParameter::cloneCADSketchParameter() const
{
    auto ncgp=new CADSketchParameter(
        script_,
        makeDefaultGeometryParameters,
        description_.simpleLatex(),
        isHidden_, isExpert_, isNecessary_, order_ );
    return ncgp;
}


Parameter *CADSketchParameter::clone() const
{
    return cloneCADSketchParameter();
}

void CADSketchParameter::operator=(const CADSketchParameter &op)
{
    description_ = op.description_;
    isHidden_ = op.isHidden_;
    isExpert_ = op.isExpert_;
    isNecessary_ = op.isNecessary_;
    order_ = op.order_;

    script_ = op.script_;
    makeDefaultGeometryParameters = op.makeDefaultGeometryParameters;
    //    cadmodel_ = op.cadmodel_;
}

bool CADSketchParameter::isDifferent(const Parameter & op) const
{
    if (const auto *sp = dynamic_cast<const CADSketchParameter*>(&op))
    {
        return script_!=sp->script();
    }
    else
        return true;
}


} // namespace insight
