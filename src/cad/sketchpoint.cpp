#include "sketchpoint.h"

#include "datum.h"
#include "constrainedsketch.h"

namespace insight {
namespace cad {


defineType(SketchPoint);
addToStaticFunctionTable(ConstrainedSketchEntity, SketchPoint, addParserRule);


SketchPoint::SketchPoint(DatumPtr plane, double x, double y)
    : plane_(plane),
    x_(x), y_(y)
{}

void SketchPoint::setCoords2D(double x, double y)
{
    x_=x;
    y_=y;
}

arma::mat SketchPoint::coords2D() const
{
    return vec2(x_, y_);
}

arma::mat SketchPoint::value() const
{
    auto pl=plane_->plane();
    return vec3(
        pl.Location()
            .Translated(pl.XDirection().XYZ()*x_)
            .Translated(pl.YDirection().XYZ()*y_)
        );
}

int SketchPoint::nDoF() const
{
    return 2;
}


double SketchPoint::getDoFValue(unsigned int iDoF) const
{
    switch (iDoF)
    {
    case 0: return x_; break;
    case 1: return y_; break;
    default:
        throw insight::Exception(
            "invalid DoF index: %d", iDoF );
    }
    return NAN;
}


void SketchPoint::setDoFValue(unsigned int iDoF, double value)
{
    switch (iDoF)
    {
    case 0: x_=value; break;
    case 1: y_=value; break;
    default:
        throw insight::Exception(
            "invalid DoF index: %d", iDoF );

    }
}

void SketchPoint::scaleSketch(double scaleFactor)
{
    x_*=scaleFactor;
    y_*=scaleFactor;
}

void SketchPoint::generateScriptCommand(
    ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{
    int myLabel=entityLabels.at(this);
    auto v = coords2D();
    script.insertCommandFor(
        myLabel,
        type() + "( "
            + boost::lexical_cast<std::string>(myLabel) + ", "
            + str(boost::format("%g, %g")%v(0)%v(1))
            + parameterString()
            + ")"
        );
}

void SketchPoint::addParserRule(ConstrainedSketchGrammar& ruleset, MakeDefaultGeometryParametersFunction)
{
    namespace qi=boost::spirit::qi;
    namespace phx=boost::phoenix;
    ruleset.entityRules.add
        (
            typeName,
            ( '('
             > qi::int_ > ','
             > qi::double_ > ',' > qi::double_
             > ruleset.r_parameters >
             ')' )
                [ qi::_val = parser::make_shared_<SketchPoint>()(ruleset.sketch->plane(), qi::_2, qi::_3),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_val, qi::_4, "."),
                 phx::insert(
                     phx::ref(ruleset.labeledEntities),
                     phx::construct<ConstrainedSketchGrammar::LabeledEntitiesMap::value_type>(qi::_1, qi::_val) ),
                 std::cout << qi::_1 ]
            );
}

std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > SketchPoint::dependencies() const
{
    return {};
}

void SketchPoint::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity> &entity,
    const std::shared_ptr<ConstrainedSketchEntity> &newEntity )
{}


} // namespace cad
} // namespace insight
