#include "distanceconstraint.h"

#include "cadparameters.h"

#include "AIS_Point.hxx"
// #include "AIS_Drawer.hxx"
#include "Prs3d_TextAspect.hxx"
#include "occtools.h"

#include "base/parameters/simpleparameter.h"
#include "constrainedsketch.h"
#include "parser.h"

using namespace std;
using namespace boost;

namespace insight {
namespace cad {



defineType(DistanceConstraint);




size_t DistanceConstraint::calcHash() const
{
    ParameterListHash h;
    h+=p1_->value();
    h+=p2_->value();
    if (distanceAlong_) h+=distanceAlong_->value();
    h+=targetValue();
    return h.getHash();
}




DistanceConstraint::DistanceConstraint(
    VectorPtr p1, VectorPtr p2,
    VectorPtr planeNormal,
    const std::string& layerName,
    VectorPtr distanceAlong)
    : ConstrainedSketchEntity(layerName),
    Distance(p1, p2, distanceAlong),
    planeNormal_(planeNormal)
{
    ParameterSet::Entries e;
    e.emplace("dimLineOfs", std::make_unique<DoubleParameter>(1., "dimension line offset"));
    e.emplace("arrowSize", std::make_unique<DoubleParameter>(1., "arrow size"));
    changeDefaultParameters(*ParameterSet::create(std::move(e), ""));
}




int DistanceConstraint::nConstraints() const
{
    return 1;
}




double DistanceConstraint::getConstraintError(unsigned int iConstraint) const
{
    insight::assertion(
        iConstraint==0,
        "invalid constraint id" );
    checkForBuildDuringAccess();
    return (distance_ - targetValue());
}




std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > DistanceConstraint::dependencies() const
{
    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > ret;

    if (auto sp1=std::dynamic_pointer_cast<ConstrainedSketchEntity>(p1_))
        ret.insert(sp1);
    if (auto sp2=std::dynamic_pointer_cast<ConstrainedSketchEntity>(p2_))
        ret.insert(sp2);

    return ret;
}




void DistanceConstraint::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity> &entity,
    const std::shared_ptr<ConstrainedSketchEntity> &newEntity )
{
    if (auto p = std::dynamic_pointer_cast<Vector>(newEntity))
    {
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(p1_) == entity)
        {
            p1_ = p;
        }
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(p2_) == entity)
        {
            p2_ = p;
        }
    }
}




arma::mat DistanceConstraint::dimLineOffset() const
{
    arma::mat dir = measureDirection();
    arma::mat n = normalized(arma::cross(dir, planeNormal_->value()));
    return n*parameters().getDouble("dimLineOfs");
}




void DistanceConstraint::setDimLineOffset(const arma::mat &p)
{
    arma::mat dir = measureDirection();
    arma::mat dir2 = p - p1_->value();
    arma::mat n = normalized(arma::cross(dir, planeNormal_->value()));

    gp_Lin lin(
        to_Pnt(p1_->value()),
        to_Vec(dir)
        );

    auto &op = parametersRef().get<DoubleParameter>("dimLineOfs");
    op.set(
        lin.Distance(to_Pnt(p))
            * (arma::dot(n,dir2)<0.?-1.:1.),
        true
        );
}




double DistanceConstraint::relativeArrowSize() const
{
    double L = std::max(SMALL, std::fabs(distance()));
    return parameters().getDouble("arrowSize")/L;
}



VectorPtr DistanceConstraint::planeNormal() const
{
    return planeNormal_;
}


bool DistanceConstraint::isInside(SelectionRect r) const
{
    return r.isInside(symbolLocation());
}




void DistanceConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const DistanceConstraint&>(other));
}




void DistanceConstraint::operator=(const DistanceConstraint& other)
{
    Distance::operator=(other);
    ConstrainedSketchEntity::operator=(other);
    planeNormal_=other.planeNormal_;
}


std::vector<vtkSmartPointer<vtkProp> >
DistanceConstraint::createActor() const
{
    return Distance::createVTKRepr(false);
}




defineType(FixedDistanceConstraint);
addToStaticFunctionTable(ConstrainedSketchEntity, FixedDistanceConstraint, addParserRule);




FixedDistanceConstraint::FixedDistanceConstraint(
    VectorPtr p1, VectorPtr p2, VectorPtr planeNormal,
    const std::string& layerName,
    VectorPtr distanceAlong
    )
    : DistanceConstraint(p1, p2, planeNormal, layerName, distanceAlong)
{
    auto ps = defaultParameters().cloneParameterSet();
    ps->insert(
        "distance",
        std::make_unique<DoubleParameter>(
            calcDistance(), "target value") );
    changeDefaultParameters(*ps);
}




double FixedDistanceConstraint::targetValue() const
{
    return parameters().getDouble("distance");
}


void FixedDistanceConstraint::setTargetValue(double dist)
{
    parametersRef().setDouble("distance", dist);
}


void FixedDistanceConstraint::scaleSketch(double scaleFactor)
{
    auto& dp=parametersRef().get<DoubleParameter>("distance");
    dp.set(dp() * scaleFactor);
}




void FixedDistanceConstraint::generateScriptCommand(
    ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{
    int myLabel=entityLabels.at(this);
    script.insertCommandFor(
        myLabel,
        type() + "( "
            + lexical_cast<std::string>(myLabel) + ", "
            + pointSpec(p1_, script, entityLabels)
            + ", "
            + pointSpec(p2_, script, entityLabels)
            + ( bool(distanceAlong_) ?
                   str(boost::format(", along [%g, %g, %g]")
                       % distanceAlong_->value()(0)
                       % distanceAlong_->value()(1)
                       % distanceAlong_->value()(2) ) : std::string("") )
            + ", layer " + layerName()
            + parameterString()
            + ")"
        );
}




void FixedDistanceConstraint::addParserRule(
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
             > ruleset.r_point > ','
             > ruleset.r_point
             > (( ',' >> qi::lit("along") > ruleset.r_vector) | qi::attr(VectorPtr()))
             > (( ',' >> qi::lit("layer") > ruleset.r_label) | qi::attr(std::string()))
             > ruleset.r_parameters >
             ')'
             )
                [ qi::_a = phx::bind(
                     &FixedDistanceConstraint::create<
                         VectorPtr, VectorPtr, VectorPtr, const std::string&, VectorPtr>,
                     qi::_2, qi::_3,
                     phx::bind(&ConstrainedSketch::sketchPlaneNormal, ruleset.sketch),
                     qi::_5, qi::_4 ),
                 phx::bind(&ConstrainedSketchParametersDelegate::changeDefaultParameters, &pd, *qi::_a),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_a, qi::_6, boost::filesystem::path(".")),
                 qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(qi::_1, qi::_a) ]
            );
}




void FixedDistanceConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const FixedDistanceConstraint&>(other));
}



ConstrainedSketchEntityPtr FixedDistanceConstraint::clone() const
{
    auto cl=FixedDistanceConstraint::create(
        p1_, p2_, planeNormal(),
        layerName(), distanceAlong_ );

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef() = parameters();
    return cl;
}




void FixedDistanceConstraint::operator=(const FixedDistanceConstraint& other)
{
    DistanceConstraint::operator=(other);
}







defineType(LinkedDistanceConstraint);
addToStaticFunctionTable(ConstrainedSketchEntity, LinkedDistanceConstraint, addParserRule);




LinkedDistanceConstraint::LinkedDistanceConstraint(
    VectorPtr p1, VectorPtr p2,
    ScalarPtr dist,
    VectorPtr planeNormal,
    const std::string& layerName,
    const std::string& distExpr,
    VectorPtr distanceAlong )

    : DistanceConstraint(p1, p2, planeNormal, layerName, distanceAlong ),
    distExpr_(distExpr), distance_(dist)
{}




double LinkedDistanceConstraint::targetValue() const
{
    return distance_->value();
}




void LinkedDistanceConstraint::scaleSketch(double scaleFactor)
{}




void LinkedDistanceConstraint::generateScriptCommand(
    ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{
    int myLabel=entityLabels.at(this);
    script.insertCommandFor(
        myLabel,
        type() + "( "
            + lexical_cast<std::string>(myLabel) + ", "
            + pointSpec(p1_, script, entityLabels) + ", "
            + pointSpec(p2_, script, entityLabels) + ", "
            + distExpr_
            + ", layer " + layerName()
            + parameterString()
            + ")"
        );
}




void LinkedDistanceConstraint::addParserRule(
    ConstrainedSketchGrammar &ruleset,
    const ConstrainedSketchParametersDelegate& pd )
{
    if (ruleset.iscadScriptRules)
    {
        typedef
            boost::spirit::qi::rule<
                std::string::iterator,
                ConstrainedSketchGrammar::ParserRuleResult(),
                insight::cad::parser::skip_grammar,
                boost::spirit::qi::locals<
                    std::shared_ptr<ConstrainedSketchEntity>,
                    insight::cad::ScalarPtr >
                > ExtParserRule;

        namespace qi=boost::spirit::qi;
        namespace phx=boost::phoenix;

        auto &rule = ruleset.addAdditionalRule(
            std::make_shared<ExtParserRule>(
                ( '('
                 > qi::int_ > ','
                 > ruleset.r_point > ','
                 > ruleset.r_point > ','
                 > qi::as_string[
                     qi::raw[ruleset.iscadScriptRules->r_scalarExpression[phx::ref(qi::_b) = qi::_1]]
        ]
                 > (( ',' >> qi::lit("layer") >> ruleset.r_label) | qi::attr(std::string()))
                 > ruleset.r_parameters >
                 ')'  )
                    [ qi::_a = phx::bind(
                         &LinkedDistanceConstraint::create<
                             VectorPtr, VectorPtr,
                             ScalarPtr, VectorPtr,
                             const std::string&, const std::string&>,
                         qi::_2, qi::_3, qi::_b,
                         phx::bind(&ConstrainedSketch::sketchPlaneNormal, ruleset.sketch),
                         qi::_5, qi::_4 ),
                     phx::bind(&ConstrainedSketchParametersDelegate::changeDefaultParameters, pd, *qi::_a),
                     phx::bind(&ConstrainedSketchEntity::parseParameterSet,
                               qi::_a, qi::_6, boost::filesystem::path(".")),
                     qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(
                         qi::_1, qi::_a ) ]
                )
            );

        ruleset.entityRules.add( typeName, rule );
    }
}




void LinkedDistanceConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const LinkedDistanceConstraint&>(other));
}

ConstrainedSketchEntityPtr LinkedDistanceConstraint::clone() const
{
    auto cl=LinkedDistanceConstraint::create(
        p1_, p2_,
        distance_, planeNormal(),
        layerName(),
        distExpr_,
        distanceAlong_ );

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef() = parameters();
    return cl;
}




void LinkedDistanceConstraint::operator=(const LinkedDistanceConstraint& other)
{
    DistanceConstraint::operator=(other);
    distance_=other.distance_;
}


} // namespace cad
} // namespace insight
