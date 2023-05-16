#include "angle.h"
#include "cadfeature.h"
#include "base/units.h"

#include "base/parameterset.h"
#include "base/parameters/simpleparameter.h"

#include "sketch.h"

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
    arma::mat d1=p1-pCtr;
    arma::mat d2=p2-pCtr;

    arma::mat n=arma::cross(d1, d2);
    if (arma::norm(n,2)<SMALL)
    {
        return 0.0;
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

//    cout<<"######### Angle Report ###########################################"<<endl;
//    cout<<"angle="<<angle_/SI::deg<<"deg"<<endl;
}

void Angle::write(ostream&) const
{}





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
                                 {"angle", new DoubleParameter(targetValue/SI::deg, "[deg] target value")}
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
    return (angle_ - targetValue())/(2.*M_PI);
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
                [ qi::_val = phx::bind(
                     &AngleConstraint::create<VectorPtr, VectorPtr, VectorPtr, double>, qi::_2, qi::_3, qi::_4, 1.0),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_val, qi::_5, "."),
                 phx::insert(
                     phx::ref(ruleset.labeledEntities),
                     phx::construct<ConstrainedSketchGrammar::LabeledEntitiesMap::value_type>(qi::_1, qi::_val)) ]
            );
}




} // namespace cad
} // namespace insight
