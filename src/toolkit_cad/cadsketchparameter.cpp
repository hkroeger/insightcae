#include "cadsketchparameter.h"

#include "base/hierarchicalelement.h"
#include "constrainedsketch.h"
#include "datum.h"
#include "base/rapidxml.h"

#include "cadpostprocactions/pointdistance.h"
#include "base/parameters/simpleparameter.h"

namespace insight {


defineType(CADSketchParameter);
addParameterFactories(CADSketchParameter);




insight::cad::ConstrainedSketch& DelayedCreatedSketch::sketchRef()
{
    if (!sketch_)
    {
        sketch_=createEmpty();
        connectSignalsToSketch(sketch_);
    }

    if (script_)
    {
        if (!script_->empty())
        {
            *sketch_ = *createSketch(*script_);
        }
        else
        {
            sketch_->clear();
        }

        script_.reset();
    }

    return *sketch_;
}

void DelayedCreatedSketch::setScript(const std::string& script)
{
    script_ = std::make_unique<std::string>(script);
}

std::string DelayedCreatedSketch::script() const
{
    if (script_)
    {
        return *script_;
    }
    else if (sketch_)
    {
        std::ostringstream so;
        sketch_->generateScript(so);
        auto s=so.str();
        return s;
    }
    else
    {
        return std::string();
    }
}


void DelayedCreatedSketch::assignFrom(
    const DelayedCreatedSketch &os)
{
    if (os.script_)
    {
        script_=std::make_unique<std::string>(*os.script_);
    }
    else
    {
        script_.reset();
    }

    if (os.sketch_)
    {
        if (!sketch_)
        {
            sketch_=createEmpty();
            connectSignalsToSketch(sketch_);
        }
        *sketch_ = *os.sketch_;
    }
    else
    {
        sketch_.reset();
    }
}








std::shared_ptr<insight::cad::ConstrainedSketch>
CADSketchParameter::createEmpty() const
{
    auto s = insight::cad::ConstrainedSketch::create(
        std::make_shared<cad::DatumPlane>(
            cad::vec3const(0,0,0),
            cad::vec3const(0,0,1) ),
        *entityProperties() );

    for (auto& ref: references_)
    {
        try
        {
            auto &r =
                const_cast<CADSketchParameter&>(*this)
                          .parentSet()
                          .get<CADGeometryParameterBase>(ref.second);

            s->setExternalReference(
                cad::ExternalReference::create(r.geometry()),
                ref.first
                );
        }
        catch (const ElementNotFoundException&)
        {}
    }

    return s;
}

void CADSketchParameter::connectSignalsToSketch(cad::ConstrainedSketchPtr s)
{
    s->setParentParameter(this);

    cad::ConstrainedSketch::GeometryEditSignal::slot_type addSlot_=
        [this](int) {
            DBG_SLOT(geometryAdded);
            if (!valueChangeSignalBlocked()) childValueChanged();
        };
    cad::ConstrainedSketch::GeometryEditSignal::slot_type removeSlot_=
        [this](int) {
            DBG_SLOT(geometryRemoved);
            if (!valueChangeSignalBlocked()) childValueChanged();
        };
    cad::ConstrainedSketch::GeometryEditSignal::slot_type changeSlot_=
        [this](int) {
            DBG_SLOT(geometryChanged);
            if (!valueChangeSignalBlocked()) childValueChanged();
        };

    addSlot_.track_foreign(s);
    addSlotConn_=s->geometryAdded.connect(addSlot_);

    removeSlot_.track_foreign(s);
    removeSlotConn_=s->geometryRemoved.connect(removeSlot_);

    changeSlot_.track_foreign(s);
    changeSlotConn_=s->geometryChanged.connect(changeSlot_);

}



cad::ConstrainedSketchPtr
CADSketchParameter::createSketch(const std::string &script) const
{
    auto newsk=createEmpty();
    std::istringstream is(script);
    newsk->readFromStream(
        is, *entityProperties()
        );
    return newsk;
}



CADSketchParameter::CADSketchParameter(
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order
    )
    : CADGeometryParameterBase(description, isHidden, isExpert, isNecessary, order)
{}




CADSketchParameter::CADSketchParameter(
    const std::string& script,
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order
    )
    : CADGeometryParameterBase(description, isHidden, isExpert, isNecessary, order)
{
    setScript(script);
}


CADSketchParameter::CADSketchParameter(
   const std::string& script,
    std::shared_ptr<insight::cad::ConstrainedSketchParametersDelegate> entityProperties,
    const std::string& presentationDelegateKey,
    const std::map<int, std::string>& references,
   const std::string& description,
   bool isHidden, bool isExpert, bool isNecessary, int order
   )
    : CADGeometryParameterBase(description, isHidden, isExpert, isNecessary, order),
    entityProperties_(
          bool(entityProperties)?
            entityProperties : insight::cad::noParametersDelegate ),
    presentationDelegateKey_(presentationDelegateKey),
    references_(references)
{
    setScript(script);
}




CADSketchParameter::~CADSketchParameter()
{}




void CADSketchParameter::setReferences(
    const std::map<int, std::string> &references )
{
    references_=references;
}


std::shared_ptr<insight::cad::ConstrainedSketchParametersDelegate>
CADSketchParameter::entityProperties() const
{
    return entityProperties_;
}

const std::string&
CADSketchParameter::presentationDelegateKey() const
{
    return presentationDelegateKey_;
}



void CADSketchParameter::setScript(const std::string& script)
{
    DelayedCreatedSketch::setScript(script);
    triggerValueChanged();
}



cad::FeaturePtr CADSketchParameter::geometry() const
{
    return const_cast<CADSketchParameter*>(this)
        ->featureGeometryRef();
}


std::string CADSketchParameter::latexRepresentation(
    const std::string&,
    int,
    const FileStorageInfo& ) const
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
    const OutputProperties& outProps
        ) const
{
    auto n = Parameter::appendToNode(name, doc, node, outProps);
    auto s=script();
    n->value(doc.allocate_string(s.c_str()));
    return n;
}

const rapidxml::xml_node<>* CADSketchParameter::readFromNode
    (
        const std::string& name,
        const rapidxml::xml_node<>& node
        )
{
    using namespace rapidxml;
    auto* child = findNode(node, name, type());
    if (child)
    {
        setScript(child->value());
    }
    return child;
}


CADSketchParameter::CADSketchParameter(
    const rapidxml::xml_node<> &node)
    : CADGeometryParameterBase(node)
{
    setScript(node.value());
}



std::unique_ptr<hierarchicalData::Element> CADSketchParameter::clone() const
{
    return std::make_unique<CADSketchParameter>(
        script(),
        entityProperties_,
        presentationDelegateKey_,
        references_,
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order() );
}



void CADSketchParameter::assignFrom(const Element& oe)
{
    auto &op = dynamic_cast<const CADSketchParameter &>(oe);

    entityProperties_ = op.entityProperties_;
    presentationDelegateKey_ = op.presentationDelegateKey_;
    references_ = op.references_;

    DelayedCreatedSketch::assignFrom(op);

    Parameter::assignFrom(op);
}



void CADSketchParameter::copyMatching( const Element& rhs )
{
    CADGeometryParameterBase::copyMatching(rhs);
}

void CADSketchParameter::extend( const Element& op )
{
    CADGeometryParameterBase::extend(op);
}

bool CADSketchParameter::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const CADSketchParameter*>(&op))
    {
        if (sketch().hash()!=oa->sketch().hash())
            return false;

        return true;
    }
    else
        return false;
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
    return sketch().size();
}


std::string CADSketchParameter::childElementName( int i, bool ) const
{
    insight::assertion(
        i>=0 && i<nChildren(),
        "invalid child parameter index %d (must be in range 0...%d)",
        i, nChildren() );

    return str(boost::format("entity_%d")%i);
}


Parameter& CADSketchParameter::childElementRef ( int i )
{
    insight::assertion(
        i>=0 && i<nChildren(),
        "invalid child parameter index %d (must be in range 0...%d)",
        i, nChildren() );

    auto g = sketch().begin();
    std::advance(g, i);
    return g->second->parametersRef();
}


const Parameter& CADSketchParameter::childElement( int i ) const
{
    insight::assertion(
        i>=0 && i<nChildren(),
        "invalid child parameter index %d (must be in range 0...%d)",
        i, nChildren() );

    auto g = sketch().begin();
    std::advance(g, i);
    return g->second->parameters();
}


} // namespace insight
