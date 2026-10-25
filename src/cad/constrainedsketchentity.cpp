#include "constrainedsketchentity.h"
#include "cadfeature.h"
#include "datum.h"
#include "base/parameters/subsetparameter.h"
#include "base/parameters/simpleparameter.h"
#include "constrainedsketch.h"

#include "base/exception.h"
#include "base/rapidxml.h"

#include "boost/functional/hash.hpp"

#include "cadparameter.h"







namespace insight {
namespace cad {


defineType(ConstrainedSketchEntity);
defineStaticFunctionTableWithArgs(
    ConstrainedSketchEntity,
    addParserRule,
    void,
    LIST(ConstrainedSketchGrammar& ruleset, const ConstrainedSketchParametersDelegate& pd),
    LIST(ruleset, pd) );


bool ConstrainedSketchEntity::SelectionRect
    ::isInside(const arma::mat& p3d) const
{
    auto p2=sketch->p3Dto2D(p3d);
    return isInside(p2[0], p2[1]);
}

bool ConstrainedSketchEntity::SelectionRect
    ::isInside(double x, double y) const
{
    return (x1<=x) && (x<=x2) && (y1<=y) && (y<=y2);
}



ConstrainedSketchEntity::ConstrainedSketchEntity(
    const std::string& layerName )
    : layerName_(
        layerName.empty() ?
            ConstrainedSketch::defaultLayerName
            : layerName ),
    defaultParameters_(ParameterSet::create()),
    parameters_(ParameterSet::create())
{
    parameters_->valueChanged.connect(
        [this]() {
            DBG_SLOT(valueChanged);
            parametersChanged(*parameters_);
        }
        );
    parameters_->childValueChanged.connect(
        [this]() {
            DBG_SLOT(childValueChanged);
            parametersChanged(*parameters_);
        }
        );
}




ConstrainedSketchEntity::~ConstrainedSketchEntity()
{}




int ConstrainedSketchEntity::nDoF() const
{
    return 0;
}




double ConstrainedSketchEntity::getDoFValue(unsigned int iDoF) const
{
    throw insight::Exception("invalid DoF index: %d", iDoF);
    return NAN;
}




void ConstrainedSketchEntity::setDoFValue(unsigned int iDoF, double value)
{
    throw insight::Exception("invalid DoF index: %d", iDoF);
}




const std::string& ConstrainedSketchEntity::layerName() const
{
    return layerName_;
}




void ConstrainedSketchEntity::setLayerName(const std::string& layerName)
{
    layerName_=layerName;
}




int ConstrainedSketchEntity::nConstraints() const
{
    return 0;
}




double ConstrainedSketchEntity::getConstraintError(unsigned int iConstraint) const
{
    throw insight::Exception("invalid constraint index: %d", iConstraint);
    return NAN;
}




size_t ConstrainedSketchEntity::hash() const
{
    size_t h=0;
    for (int i=0; i<nDoF(); ++i)
        boost::hash_combine( h, boost::hash<double>()(getDoFValue(i)) );
    return h;
}




const insight::ParameterSet& ConstrainedSketchEntity::parameters() const
{
    return *parameters_;
}




insight::ParameterSet& ConstrainedSketchEntity::parametersRef()
{
    return *parameters_;
}




const insight::ParameterSet& ConstrainedSketchEntity::defaultParameters() const
{
    return *defaultParameters_;
}



void ConstrainedSketchEntity::ensureRequiredParameters()
{}



void ConstrainedSketchEntity::changeDefaultParameters(
    const insight::ParameterSet& ps )
{
    defaultParameters_
        ->insight::ParameterSet::assignFrom(
            ps);
    // ParameterSet oldps=parameters_;
    parameters_->assignFrom(*defaultParameters_);

    ensureRequiredParameters();
}


void ConstrainedSketchEntity::parseParameterSet(
    const std::string &s,
    const boost::filesystem::path& inputFileParentPath )
{
    if (!s.empty())
    {
        insight::XMLDocument doc(s.begin(), s.end());
        if (dynamicParameterLoading_)
        {
            auto loadedPS = insight::ParameterSet::create(*doc.rootNode);
            if (parametersRef().size() > 0)
                parametersRef().copyMatching(*loadedPS);
            else
                parametersRef().assignFrom(*loadedPS);
        }
        else
        {
            parametersRef().readFromNode(std::string(), *doc.rootNode);
        }
        parametersRef().resolveRelativePaths(inputFileParentPath );
    }
}




std::string ConstrainedSketchEntity::pointSpec(
    VectorPtr p,
    ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity *, int> &entityLabels ) const
{
    if (auto sp=std::dynamic_pointer_cast<ConstrainedSketchEntity>(p))
    {
        sp->generateScriptCommand(script, entityLabels);
        return toString(entityLabels.at(sp.get()));
    }
    else
    {
#warning fragile, needs decent implementation
        auto v=p->value();
        return str(boost::format("[%g,%g,%g]")%v(0)%v(1)%v(2));
    }
}




std::string ConstrainedSketchEntity::parameterString() const
{
    std::string s;
    if (parameters().size())
    {
        s=", parameters ";
        parameters().saveToString(s);
    }
    return s;
}




void ConstrainedSketchEntity::checkParameterBindings(
    const ParameterBindings& bindings ) const
{
    for (const auto& b: bindings)
    {
        insight::assertion(
            bool(b.second.value),
            "no expression given for parameter \"%s\"", b.first.c_str() );
        try
        {
            parameters().get<DoubleParameter>(b.first);
        }
        catch (const std::exception&)
        {
            throw insight::Exception(
                "%s: cannot bind parameter \"%s\" to an expression:"
                " there is no such parameter of type double!",
                type().c_str(), b.first.c_str() );
        }
    }
}




void ConstrainedSketchEntity::setParameterBindings(
    const ParameterBindings& bindings )
{
    checkParameterBindings(bindings);
    parameterBindings_=bindings;
}




void ConstrainedSketchEntity::setParameterBinding(
    const std::string &name,
    const std::string &expression,
    ScalarPtr value )
{
    ParameterBindings b{ { name, ParameterBinding(expression, value) } };
    checkParameterBindings(b);
    parameterBindings_[name]=b.at(name);
}




boost::optional<std::string>
ConstrainedSketchEntity::parameterExpression(const std::string &name) const
{
    auto i=parameterBindings_.find(name);
    if (i!=parameterBindings_.end())
        return i->second.expression;
    return boost::none;
}




const ParameterBindings& ConstrainedSketchEntity::parameterBindings() const
{
    return parameterBindings_;
}




ParameterBindings& ConstrainedSketchEntity::parameterBindingsRef()
{
    return parameterBindings_;
}




bool ConstrainedSketchEntity::isParameterBound(const std::string &name) const
{
    return parameterBindings_.count(name)>0;
}




void ConstrainedSketchEntity::removeParameterBinding(const std::string &name)
{
    parameterBindings_.erase(name);
}




double ConstrainedSketchEntity::parameterBindingConversionFactor(const std::string&) const
{
    return 1.;
}




bool ConstrainedSketchEntity::applyParameterBindings()
{
    bool changed=false;
    for (const auto& b: parameterBindings_)
    {
        double v = b.second.value->value()
                   * parameterBindingConversionFactor(b.first);
        double cv = parameters().getDouble(b.first);
        if ( std::fabs(v-cv) > SMALL*std::max(1., std::fabs(v)) )
        {
            parametersRef().setDouble(b.first, v);
            changed=true;
        }
    }
    return changed;
}




std::string ConstrainedSketchEntity::parameterBindingString() const
{
    std::string s;
    if (parameterBindings_.size())
    {
        s=", set (";
        for (auto b=parameterBindings_.begin(); b!=parameterBindings_.end(); ++b)
        {
            if (b!=parameterBindings_.begin()) s+=", ";
            s+=b->first+" = "+b->second.expression;
        }
        s+=")";
    }
    return s;
}




bool ConstrainedSketchEntity::dependsOn(const std::weak_ptr<ConstrainedSketchEntity> &entity) const
{
    auto deps = dependencies();
    return ( deps.find(entity) != deps.end() );
}




void ConstrainedSketchEntity::operator=(const ConstrainedSketchEntity &other)
{
    defaultParameters_->assignFrom(*other.defaultParameters_);
    layerName_=other.layerName_;
    parametersRef().assignFrom( other.parameters() );
    parameterBindings_=other.parameterBindings_;
}





} // namespace cad
} // namespace insight
