
#include "base/cppextensions.h"
#include "angle.h"
#include "cadfeature.h"
#include "base/units.h"

#include "base/parameterset.h"
#include "base/parameters/simpleparameter.h"

#include "constrainedsketch.h"

namespace insight {
namespace cad {


defineType(Angle);

size_t Angle::calcHash() const
{
  ParameterListHash h;
  h+=p1_->value();
  h+=p2_->value();
  h+=pCtr_->value();
  return h.getHash();
}


Angle::Angle(insight::cad::VectorPtr p1, insight::cad::VectorPtr p2, insight::cad::VectorPtr pCtr)
: p1_(p1), p2_(p2), pCtr_(pCtr)
{}


double Angle::calculate(
        arma::mat p1,
        arma::mat p2,
        arma::mat pCtr
        )
{
    arma::mat d1 = p1 - pCtr;
    arma::mat d2 = p2 - pCtr;

    if ( (arma::norm(d1, 2)<SMALL) || (arma::norm(d2, 2)<SMALL) ) // might appear during sketch resolution
    {
        return 100.*M_PI;
    }

    arma::mat n=arma::cross(d1, d2);
    if (arma::norm(n,2)<SMALL)
    {
        return 0.;
    }
    else
    {
        arma::mat ex=normalized(d1);
        arma::mat ey=normalized(arma::cross(n,ex));
        return atan2(arma::dot(d2, ey), arma::dot(d2, ex));
    }
}


void Angle::build()
{
    angle_ = calculate(
                p1_->value(),
                p2_->value(),
                pCtr_->value() );
}

void Angle::write(ostream&) const
{}

void Angle::operator=(const Angle &other)
{
    p1_=other.p1_;
    p2_=other.p2_;
    pCtr_=other.pCtr_;
    angle_=other.angle_;
    PostprocAction::operator=(other);
}





defineType(AngleConstraint);
addToStaticFunctionTable(ConstrainedSketchEntity, AngleConstraint, addParserRule);

size_t AngleConstraint::calcHash() const
{
    ParameterListHash h;
    h+=p1_->value();
    h+=p2_->value();
    h+=pCtr_->value();
    h+=targetValue();
    return h.getHash();
}


AngleConstraint::AngleConstraint(VectorPtr p1, VectorPtr p2, VectorPtr pCtr, double targetValue)
    : Angle(p1, p2, pCtr)
{
    changeDefaultParameters(
                ParameterSet({
                      {"angle", std::make_shared<DoubleParameter>(targetValue/SI::deg, "[deg] target value")}
                })
            );
}

AngleConstraint::AngleConstraint(VectorPtr p1, VectorPtr p2, VectorPtr pCtr)
    : Angle(p1, p2, pCtr)
{
    changeDefaultParameters(
        ParameterSet({
                      {"angle", std::make_shared<DoubleParameter>(
                calculate(
                    p1_->value(),
                    p2_->value(),
                    pCtr_->value() )/SI::deg,
                    "[deg] target value")}
        })
    );
}


double AngleConstraint::targetValue() const
{
    return parameters().getDouble("angle")*SI::deg;
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

void AngleConstraint::generateScriptCommand(
    ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{
    int myLabel=entityLabels.at(this);
    script.insertCommandFor(
        myLabel,
        type() + "( "
            + boost::lexical_cast<std::string>(myLabel) + ", "
            + pointSpec(p1_, script, entityLabels)
            + ", "
            + pointSpec(p2_, script, entityLabels)
            + ", "
            + pointSpec(pCtr_, script, entityLabels)
            + parameterString()
            + ")"
        );
}


void AngleConstraint::addParserRule(ConstrainedSketchGrammar &ruleset, MakeDefaultGeometryParametersFunction)
{
    namespace qi = boost::spirit::qi;
    namespace phx = boost::phoenix;
    ruleset.entityRules.add
        (
            typeName,
            ( '('
             > qi::int_ > ','
             > ruleset.r_point > ','
             > ruleset.r_point > ','
             > ruleset.r_point
             > ruleset.r_parameters >
             ')'
             )
           [ qi::_a = phx::bind(
                 &AngleConstraint::create<VectorPtr, VectorPtr, VectorPtr, double>, qi::_2, qi::_3, qi::_4, 1.0),
             phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_a, qi::_5, boost::filesystem::path(".")),
             qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(qi::_1, qi::_a) ]
        );
}

std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > AngleConstraint::dependencies() const
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
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(pCtr_) == entity)
        {
            pCtr_ = p;
        }
    }
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



} // namespace cad
} // namespace insight
