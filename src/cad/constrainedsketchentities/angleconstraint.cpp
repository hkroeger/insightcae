#include "angleconstraint.h"

#include "base/units.h"
#include "base/parameterset.h"
#include "base/parameters/simpleparameter.h"

#include "constrainedsketch.h"

namespace insight {
namespace cad {


defineType(AngleConstraint);

size_t AngleConstraint::calcHash() const
{
    ParameterListHash h;
    h+=p1_->value();
    h+=p2_->value();
    h+=pCtr_->value();
    h+=targetValue();
    return h.getHash();
}


AngleConstraint::AngleConstraint(
    VectorPtr p1, VectorPtr p2, VectorPtr pCtr,
    const std::string& layerName
    )
    : ConstraintWithDimensionLines(layerName),
    Angle(
        p1,
        p2 ?
            p2 :
            std::make_shared<insight::cad::AddedVector>(
                pCtr,
                insight::cad::vec3const(1,0,0) ),
        pCtr )
{
    ParameterSet::Entries e;
    e.emplace("dimLineRadius", std::make_unique<DoubleParameter>(1., "dimension line radius"));
    e.emplace("arrowSize", std::make_unique<DoubleParameter>(1., "arrow size"));
    changeDefaultParameters(*ParameterSet::create(std::move(e), ""));
}



int AngleConstraint::nConstraints() const
{
    return 1;
}

double AngleConstraint::getConstraintError(unsigned int iConstraint) const
{
    insight::assertion(
        iConstraint==0,
        "invalid constraint id" );
    checkForBuildDuringAccess();
    return (angle_ - targetValue())/M_PI;
}

void AngleConstraint::scaleSketch(double scaleFactor)
{}

std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> >
AngleConstraint::dependencies() const
{
    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > ret;

    if (auto sp1=std::dynamic_pointer_cast<ConstrainedSketchEntity>(p1_))
        ret.insert(sp1);
    if (auto sp2=std::dynamic_pointer_cast<ConstrainedSketchEntity>(p2_))
        ret.insert(sp2);
    if (auto spCtr=std::dynamic_pointer_cast<ConstrainedSketchEntity>(pCtr_))
        ret.insert(spCtr);

    return ret;
}

void AngleConstraint::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity> &entity,
    const std::shared_ptr<ConstrainedSketchEntity> &newEntity)
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
        else if (auto adder = std::dynamic_pointer_cast<AddedVector>(p2_))
        {
            auto po=std::dynamic_pointer_cast<SketchPoint>(entity.lock());
            if (adder->p1()==po)
            {
                adder->p1()=po;
            }
        }
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(pCtr_) == entity)
        {
            pCtr_ = p;
        }
    }
}

double AngleConstraint::dimLineRadius() const
{
    return parameters().get<DoubleParameter>("dimLineRadius")();
}

void AngleConstraint::setDimLineRadius(double r)
{
    auto &op = parametersRef().get<DoubleParameter>("dimLineRadius");
    op.set( r, true );
}

double AngleConstraint::relativeArrowSize() const
{
    return parameters().getDouble("arrowSize")
           /std::max(insight::LSMALL, angle_)
           /std::max(insight::LSMALL, dimLineRadius() );
}

void AngleConstraint::setArrowSize(double absoluteArrowSize)
{
    parametersRef().setDouble("arrowSize", absoluteArrowSize);
}



bool AngleConstraint::isInside( SelectionRect r) const
{
    return r.isInside(symbolLocation());
}



void AngleConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const AngleConstraint&>(other));
}


void AngleConstraint::operator=(const AngleConstraint& other)
{
    Angle::operator=(other);
    ConstrainedSketchEntity::operator=(other);
}


std::vector<vtkSmartPointer<vtkProp> > AngleConstraint::createActor() const
{
    return Angle::createVTKRepr();
}






defineType(FixedAngleConstraint);
addToStaticFunctionTable(ConstrainedSketchEntity, FixedAngleConstraint, addParserRule);




FixedAngleConstraint::FixedAngleConstraint(
    VectorPtr p1, VectorPtr p2, VectorPtr pCtr,
    const std::string& layerName
    )
    : AngleConstraint(p1, p2, pCtr, layerName)
{
    auto ps = defaultParameters().cloneParameterSet();
    ps->insert(
        "angle", std::make_unique<DoubleParameter>(
                  calculate(
                    p1_->value(),
                    p2_->value(),
                    pCtr_->value() )/SI::deg, "[deg] target value"));
    changeDefaultParameters(*ps);
}




double FixedAngleConstraint::targetValue() const
{
    return parameters().getDouble("angle")*SI::deg;
}

void FixedAngleConstraint::setTargetValue(double angle)
{
    parametersRef().setDouble("angle", angle/SI::deg);
}




void FixedAngleConstraint::generateScriptCommand(
    ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{
    int myLabel=entityLabels.at(this);
    script.insertCommandFor(
        myLabel,
        type() + "( "
            + boost::lexical_cast<std::string>(myLabel) + ", "
            + pointSpec(p1_, script, entityLabels) + ", "
            + ( std::dynamic_pointer_cast<AddedVector>(p2_) ?
                   "toHorizontal" :
                   pointSpec(p2_, script, entityLabels) ) + ", "
            + pointSpec(pCtr_, script, entityLabels)
            + ", layer " + layerName()
            + parameterString()
            + ")"
        );
}




void FixedAngleConstraint::addParserRule(
    ConstrainedSketchGrammar &ruleset,
    const ConstrainedSketchParametersDelegate& pd )
{
    namespace qi = boost::spirit::qi;
    namespace phx = boost::phoenix;
    ruleset.entityRules.add
        (
            typeName,
            ( '(' > qi::int_ > ','
             > ruleset.r_point > ','
             > ( ( qi::lit("toHorizontal") >> qi::attr(VectorPtr()) ) | (ruleset.r_point) ) > ','
             > ruleset.r_point
             > (( ',' >> qi::lit("layer") >> ruleset.r_label) | qi::attr(std::string()))
             > ruleset.r_parameters >
             ')' )
                [ qi::_a = phx::bind(
                     &FixedAngleConstraint::create<
                         VectorPtr, VectorPtr, VectorPtr, const std::string&>,
                     qi::_2, qi::_3, qi::_4, qi::_5),
                 phx::bind(&ConstrainedSketchParametersDelegate::changeDefaultParameters, &pd, *qi::_a),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_a, qi::_6, boost::filesystem::path(".")),
                 qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(qi::_1, qi::_a) ]
            );
}




void FixedAngleConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const FixedAngleConstraint&>(other));
}

ConstrainedSketchEntityPtr FixedAngleConstraint::clone() const
{
    auto cl=FixedAngleConstraint::create(
        p1_,
        std::dynamic_pointer_cast<AddedVector>(p2_)?nullptr:p2_,
        pCtr_,
        layerName() );

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef() = parameters();
    return cl;
}




void FixedAngleConstraint::operator=(const FixedAngleConstraint& other)
{
    AngleConstraint::operator=(other);
}








defineType(LinkedAngleConstraint);
addToStaticFunctionTable(ConstrainedSketchEntity, LinkedAngleConstraint, addParserRule);




LinkedAngleConstraint::LinkedAngleConstraint(
    VectorPtr p1, VectorPtr p2, VectorPtr pCtr,
    ScalarPtr angle,
    const std::string& layerName,
    const std::string& angleExpr
    )
    : AngleConstraint(p1, p2, pCtr, layerName),
    angleExpr_(angleExpr),
    angle_(angle)
{}



double LinkedAngleConstraint::targetValue() const
{
    return angle_->value();
}



void LinkedAngleConstraint::generateScriptCommand(
    ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{
    int myLabel=entityLabels.at(this);
    script.insertCommandFor(
        myLabel,
        type() + "( "
            + boost::lexical_cast<std::string>(myLabel) + ", "
            + pointSpec(p1_, script, entityLabels) + ", "
            + pointSpec(p2_, script, entityLabels) + ", "
            + pointSpec(pCtr_, script, entityLabels) + ", "
            + angleExpr_
            + ", layer " + layerName()
            + parameterString()
            + ")"
        );
}




void LinkedAngleConstraint::addParserRule(
    ConstrainedSketchGrammar &ruleset,
    const ConstrainedSketchParametersDelegate& pd )
{
    if (ruleset.iscadScriptRules)
    {
        namespace qi = boost::spirit::qi;
        namespace phx = boost::phoenix;

        typedef
            boost::spirit::qi::rule<
                std::string::iterator,
                ConstrainedSketchGrammar::ParserRuleResult(),
                insight::cad::parser::skip_grammar,
                boost::spirit::qi::locals<
                    std::shared_ptr<ConstrainedSketchEntity>,
                    insight::cad::ScalarPtr >
                > ExtParserRule;

        auto &rule = ruleset.addAdditionalRule(
            std::make_shared<ExtParserRule>(
                ( '('
                 > qi::int_ > ','
                 > ruleset.r_point > ','
                 > ruleset.r_point > ','
                 > ruleset.r_point > ','
                 > qi::as_string[
                     qi::raw[ruleset.iscadScriptRules->r_scalarExpression[phx::ref(qi::_b) = qi::_1]]
        ]
                 > (( ',' >> qi::lit("layer") >> ruleset.r_label) | qi::attr(std::string()))
                 > ruleset.r_parameters >
                 ')'
                 )
                    [ qi::_a = phx::bind(
                         &LinkedAngleConstraint::create<
                             VectorPtr, VectorPtr, VectorPtr, ScalarPtr,
                             const std::string&, const std::string&>,
                         qi::_2, qi::_3, qi::_4, qi::_b, qi::_6, qi::_5),
                     phx::bind(&ConstrainedSketchParametersDelegate::changeDefaultParameters, pd, *qi::_a),
                     phx::bind(&ConstrainedSketchEntity::parseParameterSet,
                               qi::_a, qi::_7, boost::filesystem::path(".")),
                     qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(qi::_1, qi::_a) ]
                )
            );

        ruleset.entityRules.add(typeName, rule);
    }
}




void LinkedAngleConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const LinkedAngleConstraint&>(other));
}

ConstrainedSketchEntityPtr LinkedAngleConstraint::clone() const
{
    auto cl=LinkedAngleConstraint::create(
        p1_,
        std::dynamic_pointer_cast<AddedVector>(p2_)?nullptr:p2_,
        pCtr_,
        angle_,
        layerName(), angleExpr_ );

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef() = parameters();
    return cl;
}




void LinkedAngleConstraint::operator=(const LinkedAngleConstraint& other)
{
    AngleConstraint::operator=(other);
    angleExpr_=other.angleExpr_;
    angle_=other.angle_;
}



} // namespace cad
} // namespace insight
