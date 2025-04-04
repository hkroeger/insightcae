#include "constrainedsketchentity.h"
#include "base/parameters/subsetparameter.h"
#include "constrainedsketch.h"

#include "base/exception.h"

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
{}




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



void ConstrainedSketchEntity::changeDefaultParameters(
    const insight::ParameterSet& ps )
{
    defaultParameters_
        ->insight::ParameterSet::operator=(
            ps);
    // ParameterSet oldps=parameters_;
    *parameters_=*defaultParameters_;

#warning copy values from old; merge is not the right function
    //parameters_.merge(oldps);
}


void ConstrainedSketchEntity::parseParameterSet(
    const std::string &s,
    const boost::filesystem::path& inputFileParentPath )
{
    if (!s.empty())
    {
        using namespace rapidxml;
        xml_document<> doc;
        doc.parse<0>(const_cast<char*>(&s[0]));
        xml_node<> *rootnode = doc.first_node("root");

        parametersRef().readFromNode(std::string(), *rootnode, inputFileParentPath );
//        std::cout<<parameters_<<std::endl;
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
        return boost::lexical_cast<std::string>(entityLabels.at(sp.get()));
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
        parameters().saveToString(
            s,
            boost::filesystem::current_path()
                / "outfile"
            );
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
    *defaultParameters_ = *other.defaultParameters_;
    parametersRef() = other.parameters();
}





} // namespace cad
} // namespace insight
