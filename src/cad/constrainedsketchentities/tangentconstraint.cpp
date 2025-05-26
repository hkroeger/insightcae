#include "tangentconstraint.h"

#include "base/exception.h"
#include "constrainedsketch.h"
#include "constrainedsketchgrammar.h"


namespace insight
{
namespace cad
{


defineType(TangentConstraint);


Vector *TangentConstraint::commonPoint() const
{
    auto cp = line1_->start();
    if (cp==line2_->start()) return cp.get();
    if (cp==line2_->end()) return cp.get();
    cp = line1_->end();
    if (cp==line2_->start()) return cp.get();
    if (cp==line2_->end()) return cp.get();
    throw insight::Exception("lines do not have a common point!");
    return nullptr;
}


TangentConstraint::TangentConstraint(
    std::shared_ptr<Line> line1,
    std::shared_ptr<Line> line2,
    const std::string& layerName)
: SingleSymbolConstraint(layerName),
  line1_(line1), line2_(line2)
{}

std::string TangentConstraint::symbolText() const
{
    return "T";
}

arma::mat TangentConstraint::symbolLocation() const
{
    return commonPoint()->value();
}


int TangentConstraint::nConstraints() const
{
    return 1;
}

double TangentConstraint::getConstraintError(unsigned int iConstraint) const
{
#warning needs to be generalized for curves
    arma::mat d1 = normalized(line1_->end()->value() - line1_->start()->value());
    arma::mat d2 = normalized(line2_->end()->value() - line2_->start()->value());
    //auto err = pow(arma::dot(d1, d2),2) - 1.; // bad convergence
    auto err=arma::norm(arma::cross(d1, d2), 2);
    // std::cout<<"d1="<<d1.t()<<" d2="<<d2.t()<<" err="<<err<<std::endl;
    return err;
}

void TangentConstraint::scaleSketch(double scaleFactor)
{}


void TangentConstraint::generateScriptCommand(
    ConstrainedSketchScriptBuffer& script,
    const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const
{
    int myLabel=entityLabels.at(this);

    line1_->generateScriptCommand(script, entityLabels);
    line2_->generateScriptCommand(script, entityLabels);

    script.insertCommandFor(
        myLabel,
        type() + "( "
            + boost::lexical_cast<std::string>(myLabel) + ", "
            + boost::lexical_cast<std::string>(entityLabels.at(line1_.get())) + ", "
            + boost::lexical_cast<std::string>(entityLabels.at(line2_.get()))
            + ", layer " + layerName()
            + parameterString()
            + ")"
        );
}


addToStaticFunctionTable(ConstrainedSketchEntity, TangentConstraint, addParserRule);



void TangentConstraint::addParserRule(
    ConstrainedSketchGrammar& ruleset,
    const ConstrainedSketchParametersDelegate& pd )
{
    using namespace insight::cad;
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
                [
                 qi::_a = phx::bind(
                     &TangentConstraint::create<
                        std::shared_ptr<Line>,
                        std::shared_ptr<Line>,
                        const std::string& >,
                     phx::bind(&ConstrainedSketch::get<Line>, ruleset.sketch, qi::_2),
                     phx::bind(&ConstrainedSketch::get<Line>, ruleset.sketch, qi::_3),
                    qi::_4 ),
                 phx::bind(&ConstrainedSketchParametersDelegate::changeDefaultParameters, &pd, *qi::_a),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_a, qi::_5, "."),

                 qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(
                        qi::_1, qi::_a)
                ]
            );
}


std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> >
TangentConstraint::dependencies() const
{
    return { line1_, line2_ };
}


void TangentConstraint::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity>& entity,
    const std::shared_ptr<ConstrainedSketchEntity>& newEntity)
{
    if (auto l = std::dynamic_pointer_cast<Line>(newEntity))
    {
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(line1_) == entity)
        {
            line1_ = l;
        }
        else if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(line2_) == entity)
        {
            line2_ = l;
        }
    }
}



void TangentConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const TangentConstraint&>(other));
}

void TangentConstraint::operator=(const TangentConstraint& other)
{
    line1_=other.line1_;
    line2_=other.line2_;
    ConstrainedSketchEntity::operator=(other);
}


ConstrainedSketchEntityPtr TangentConstraint::clone() const
{
    auto cl=TangentConstraint::create( line1_, line2_, layerName() );

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef() = parameters();
    return cl;
}


}
}
