#include "cadsketchparameter.h"

#include "constrainedsketch.h"
#include "datum.h"
#include "base/rapidxml.h"

#include "cadpostprocactions/pointdistance.h"
#include "base/parameters/simpleparameter.h"

namespace insight {


defineType(CADSketchParameter);
addToFactoryTable(Parameter, CADSketchParameter);










void CADSketchParameter::resetCADGeometry()
{
    CADGeometry_ = insight::cad::ConstrainedSketch::create(
        std::make_shared<cad::DatumPlane>(
            cad::vec3const(0,0,0),
            cad::vec3const(0,0,1) ));

    CADGeometry_->geometryAdded.connect(
        cad::ConstrainedSketch::GeometryEditSignal::slot_type(
            [this](int) {
                if (!valueChangeSignalBlocked_) childValueChanged();
            }
            ).track_foreign(CADGeometry_)
        );
    CADGeometry_->geometryRemoved.connect(
        cad::ConstrainedSketch::GeometryEditSignal::slot_type(
            [this](int) {
                if (!valueChangeSignalBlocked_) childValueChanged();
            }
            ).track_foreign(CADGeometry_)
        );
    CADGeometry_->geometryChanged.connect(
        cad::ConstrainedSketch::GeometryEditSignal::slot_type(
            [this](int) {
                if (!valueChangeSignalBlocked_) childValueChanged();
            }
            ).track_foreign(CADGeometry_)
        );

    for (auto& ref: references_)
    {
        try
        {
            auto &r = parentSet().get<CADGeometryParameter>(ref.second);
            CADGeometry_->setExternalReference(
                cad::ExternalReference::create(r.geometry()),
                ref.first
                );
        }
        catch (const ParameterNotFoundException&)
        {}
    }
}



CADSketchParameter::CADSketchParameter(
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order
    )
    : CADGeometryParameter(description, isHidden, isExpert, isNecessary, order)
{}




CADSketchParameter::CADSketchParameter(
    const std::string& script,
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order
    )
    : CADGeometryParameter(description, isHidden, isExpert, isNecessary, order)
{
    setScript(script);
}


CADSketchParameter::CADSketchParameter(
   const std::string& script,
    cad::MakeDefaultGeometryParametersFunction mdpf,
    const std::map<int, std::string>& references,
   const std::string& description,
   bool isHidden, bool isExpert, bool isNecessary, int order
   )
    : CADGeometryParameter(description, isHidden, isExpert, isNecessary, order),
    makeDefaultGeometryParameters(mdpf),
    references_(references)
{
    setScript(script);
}

void CADSketchParameter::setReferences(
    const std::map<int, std::string> &references )
{
    references_=references;
}



ParameterSet CADSketchParameter::defaultGeometryParameters() const
{
    return makeDefaultGeometryParameters();
}



std::string CADSketchParameter::script() const
{
    if (script_)
    {
        return *script_;
    }
    else if (CADGeometry_)
    {
        std::ostringstream so;
        CADGeometry_->generateScript(so);
        return so.str();;
    }
    else
    {
        return std::string();
    }
}


void CADSketchParameter::setScript(const std::string& script)
{
    if (script.empty())
    {
        script_.reset();
        CADGeometry_.reset();
    }
    else
    {
        script_ =
            std::make_unique<std::string>(script);
    }

    triggerValueChanged();
}


const insight::cad::ConstrainedSketch&
CADSketchParameter::featureGeometry() const
{
    return *const_cast<CADSketchParameter*>(this)
                ->featureGeometryRef();
}

std::shared_ptr<insight::cad::ConstrainedSketch>
CADSketchParameter::featureGeometryRef()
{
    if (!CADGeometry_)
    {
        resetCADGeometry();
    }

    if (script_)
    {
        if (!script_->empty())
        {
            std::istringstream is(*script_);

            CADGeometry_->readFromStream(
                is, makeDefaultGeometryParameters()
                );

            script_.reset();
        }
        else
        {
            CADGeometry_->clear();
        }
    }

    return CADGeometry_;
}

cad::FeaturePtr CADSketchParameter::geometry() const
{
    return const_cast<CADSketchParameter*>(this)
        ->featureGeometryRef();
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
    auto s=script();
    n->value(doc.allocate_string(s.c_str()));
    return n;
}

void CADSketchParameter::readFromNode
    (
        const std::string& name,
        rapidxml::xml_node<>& node,
        boost::filesystem::path inputfilepath
        )
{
    using namespace rapidxml;
    xml_node<>* child = findNode(node, name, type());
    if (child)
    {
        setScript(child->value());
    }
}

CADSketchParameter *
CADSketchParameter::cloneCADSketchParameter(
    bool keepParentRef
    ) const
{
    auto ncgp=new CADSketchParameter(
        script(),
        makeDefaultGeometryParameters,
        references_,
        description_.simpleLatex(),
        isHidden_, isExpert_, isNecessary_, order_ );

    if (keepParentRef)
        ncgp->setParent(parent());

    return ncgp;
}


Parameter *CADSketchParameter::clone() const
{
    return cloneCADSketchParameter();
}

void CADSketchParameter::copyFrom(const Parameter& op)
{
    operator=(dynamic_cast<const CADSketchParameter&>(op));
}

void CADSketchParameter::operator=(const CADSketchParameter &op)
{
    description_ = op.description_;
    isHidden_ = op.isHidden_;
    isExpert_ = op.isExpert_;
    isNecessary_ = op.isNecessary_;
    order_ = op.order_;

    makeDefaultGeometryParameters = op.makeDefaultGeometryParameters;
    references_ = op.references_;

    if (op.script_)
    {
        script_=std::make_unique<std::string>(*op.script_);
    }
    else
    {
        script_.reset();
    }

    if (op.CADGeometry_)
    {
        resetCADGeometry();
        *CADGeometry_ = *op.CADGeometry_;
    }
    else
    {
        CADGeometry_.reset();
    }

    triggerValueChanged();
}

bool CADSketchParameter::isDifferent(const Parameter & op) const
{
    if (const auto *sp = dynamic_cast<const CADSketchParameter*>(&op))
    {
        return script()!=sp->script();
    }
    else
        return true;
}


int CADSketchParameter::nChildren() const
{
    if (!script_ && !CADGeometry_)
        return 0;
    return featureGeometry().size();
}


std::string CADSketchParameter::childParameterName( int i ) const
{
    insight::assertion(
        i>=0 && i<nChildren(),
        "invalid child parameter index %d (must be in range 0...%d)",
        i, nChildren() );

    return str(boost::format("entity_%d")%i);
}


Parameter& CADSketchParameter::childParameterRef ( int i )
{
    insight::assertion(
        i>=0 && i<nChildren(),
        "invalid child parameter index %d (must be in range 0...%d)",
        i, nChildren() );

    auto g = featureGeometry().begin();
    std::advance(g, i);
    return g->second->parametersRef();
}


const Parameter& CADSketchParameter::childParameter( int i ) const
{
    insight::assertion(
        i>=0 && i<nChildren(),
        "invalid child parameter index %d (must be in range 0...%d)",
        i, nChildren() );

    auto g = featureGeometry().begin();
    std::advance(g, i);
    return g->second->parameters();
}


} // namespace insight
