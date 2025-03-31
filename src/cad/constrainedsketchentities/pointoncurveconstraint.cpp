#include "pointoncurveconstraint.h"

#include "cadfeatures/line.h"

#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"



namespace insight
{
namespace cad
{


defineType(PointOnCurveConstraint);




PointOnCurveConstraint::PointOnCurveConstraint(
    std::shared_ptr<insight::cad::SketchPoint> p,
    std::shared_ptr<insight::cad::Feature> curve,
    const std::string& layerName )
    : SingleSymbolConstraint(layerName),
    p_(p),
    curve_(curve)
{}


std::string PointOnCurveConstraint::symbolText() const
{
    return "C";
}

arma::mat PointOnCurveConstraint::symbolLocation() const
{
    return p_->value();
}




int PointOnCurveConstraint::nConstraints() const
{
    return 1;
}




double PointOnCurveConstraint::getConstraintError(unsigned int iConstraint) const
{
    insight::assertion(
        iConstraint==0,
        "invalid constraint index");

    // if (auto line = std::dynamic_pointer_cast<insight::cad::Line>(curve_))
    // {
    //     arma::mat a = line->start()->value();
    //     arma::mat b = insight::normalized(line->end()->value() - a);
    //     arma::mat p = p_->value();
    //     double nom=arma::norm( arma::cross(p-a, b), 2 );
    //     double denom=arma::norm(b, 2);
    //     if (fabs(denom)<insight::SMALL)
    //         return 1./insight::LSMALL;
    //     else
    //         return nom/denom;
    // }
    // else
    // {
        return curve_->minDist(p_->value());
    // }
}




void PointOnCurveConstraint::scaleSketch(double scaleFactor)
{}




void PointOnCurveConstraint::generateScriptCommand(
    insight::cad::ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{
    int myLabel=entityLabels.at(this);

    auto sc = std::dynamic_pointer_cast<insight::cad::ConstrainedSketchEntity>(curve_);
    insight::assertion( bool(sc),
                       "not implemented: script generation with non-sketch-curve");

    sc->generateScriptCommand(script, entityLabels);

    script.insertCommandFor(
        myLabel,
        type() + "( "
            + boost::lexical_cast<std::string>(myLabel) + ", "
            + boost::lexical_cast<std::string>(entityLabels.at(sc.get())) + ", "
            + pointSpec(p_, script, entityLabels)
            + ", layer " + layerName()
            + parameterString()
            + ")"
        );
}




namespace insight { namespace cad {
addToStaticFunctionTable(ConstrainedSketchEntity, PointOnCurveConstraint, addParserRule);
}}




void PointOnCurveConstraint::addParserRule(
    ConstrainedSketchGrammar &ruleset,
    const ConstrainedSketchParametersDelegate& pd )
{
    namespace qi=boost::spirit::qi;
    namespace phx=boost::phoenix;
    ruleset.entityRules.add
        (
            typeName,
            ( '('
             > qi::int_ > ','
             > qi::int_ > ','
             > qi::int_
             > (( ',' >> qi::lit("layer") >> ruleset.r_label) | qi::attr(std::string()))
             > ruleset.r_parameters >
             ')'
         )
         [ qi::_a = phx::bind(
                 &PointOnCurveConstraint::create<SketchPointPtr, FeaturePtr, const std::string&>,
                 phx::bind(&ConstrainedSketch::get<SketchPoint>, ruleset.sketch, qi::_3),
                 phx::bind(&ConstrainedSketch::get<Feature>, ruleset.sketch, qi::_2), qi::_4 ),
             phx::bind(&ConstrainedSketchParametersDelegate::changeDefaultParameters, &pd, *qi::_a),
             phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_a, qi::_5, "."),
             qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(qi::_1, qi::_a) ]
        );
}




std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> >
PointOnCurveConstraint::dependencies() const
{
    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > ret
        { p_ };

    if (auto l = std::dynamic_pointer_cast<ConstrainedSketchEntity>(curve_))
        ret.insert(l);

    return ret;
}




void PointOnCurveConstraint::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity> &entity,
    const std::shared_ptr<ConstrainedSketchEntity> &newEntity)
{
    if (auto p = std::dynamic_pointer_cast<SketchPoint>(newEntity))
    {
        if ( std::dynamic_pointer_cast<ConstrainedSketchEntity>(p_) == entity )
        {
            p_ = p;
        }
    }
    if (auto c = std::dynamic_pointer_cast<Feature>(newEntity))
    {
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(curve_) == entity)
        {
            curve_ = c;
        }
    }
}




void PointOnCurveConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const PointOnCurveConstraint&>(other));
}



ConstrainedSketchEntityPtr PointOnCurveConstraint::clone() const
{
    auto cl=PointOnCurveConstraint::create( p_, curve_, layerName() );

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef() = parameters();
    return cl;
}




void PointOnCurveConstraint::operator=(const PointOnCurveConstraint& other)
{
    p_=other.p_;
    curve_=other.curve_;
    ConstrainedSketchEntity::operator=(other);
}

}
}
