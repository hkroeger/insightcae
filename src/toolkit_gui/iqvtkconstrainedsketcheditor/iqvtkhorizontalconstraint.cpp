#include "iqvtkhorizontalconstraint.h"

#include "constrainedsketch.h"
#include "constrainedsketchgrammar.h"

#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"


using namespace insight::cad;


defineType(IQVTKHorizontalConstraint);




IQVTKHorizontalConstraint::IQVTKHorizontalConstraint(
    std::shared_ptr<insight::cad::Line> line,
    const std::string& layerName )
    : IQVTKConstrainedSketchEntity(layerName), line_(line)
{}




std::vector<vtkSmartPointer<vtkProp> >
IQVTKHorizontalConstraint::createActor() const
{
    auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
    caption->SetCaption("H");
    caption->BorderOff();
    caption->SetAttachmentPoint(
        arma::mat(0.5*
                  (line_->getDatumPoint("p0")+line_->getDatumPoint("p1")))
            .memptr());
    caption->GetTextActor()->SetTextScaleModeToNone(); //key: fix the font size
    caption->GetCaptionTextProperty()->SetColor(0,0,0);
    caption->GetCaptionTextProperty()->SetFontSize(10);
    caption->GetCaptionTextProperty()->FrameOff();
    caption->GetCaptionTextProperty()->ShadowOff();
    caption->GetCaptionTextProperty()->BoldOff();

    return {caption};
}




int IQVTKHorizontalConstraint::nConstraints() const
{
    return 1;
}




double IQVTKHorizontalConstraint::getConstraintError(
    unsigned int iConstraint) const
{
    auto p0 = std::dynamic_pointer_cast<SketchPoint>(line_->start());
    auto p1 = std::dynamic_pointer_cast<SketchPoint>(line_->end());
    insight::assertion(bool(p0), "only lines with sketch end points are allowed!");
    insight::assertion(bool(p1), "only lines with sketch end points are allowed!");
    arma::mat p02 = p0->coords2D();
    arma::mat p12 = p1->coords2D();
    arma::mat d2 = p12-p02;
    return d2(1);
}




void IQVTKHorizontalConstraint::scaleSketch(double scaleFactor)
{}




void IQVTKHorizontalConstraint::generateScriptCommand(
    insight::cad::ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity *, int> &entityLabels
    ) const
{
    int myLabel=entityLabels.at(this);

    line_->generateScriptCommand(script, entityLabels);

    script.insertCommandFor(
        myLabel,
        type() + "( "
            + boost::lexical_cast<std::string>(myLabel) + ", "
            + boost::lexical_cast<std::string>(entityLabels.at(line_.get()))
            + ", layer " + layerName()
            + parameterString()
            + ")"
        );
}




namespace insight { namespace cad {
addToStaticFunctionTable(ConstrainedSketchEntity, IQVTKHorizontalConstraint, addParserRule);
}}




void IQVTKHorizontalConstraint::addParserRule(
    insight::cad::ConstrainedSketchGrammar &ruleset,
    insight::cad::MakeDefaultGeometryParametersFunction )
{
    using namespace insight::cad;
    namespace qi=boost::spirit::qi;
    namespace phx=boost::phoenix;
    ruleset.entityRules.add
        (
            typeName,
            ( '('
             > qi::int_ > ','
             > qi::int_
             > (( ',' >> qi::lit("layer") >> ruleset.r_label) | qi::attr(std::string()))
             > ruleset.r_parameters >
             ')'
         )
            [ qi::_a = phx::bind(
                 &IQVTKHorizontalConstraint::create<std::shared_ptr<insight::cad::Line>, const std::string& >,
                 phx::bind(&ConstrainedSketch::get<Line>, ruleset.sketch, qi::_2), qi::_3 ),
              phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_a, qi::_4, "."),
              qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(qi::_1, qi::_a) ]
            );
}




std::set<std::comparable_weak_ptr<insight::cad::ConstrainedSketchEntity> > IQVTKHorizontalConstraint::dependencies() const
{
    return { line_ };
}




void IQVTKHorizontalConstraint::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity> &entity,
    const std::shared_ptr<ConstrainedSketchEntity> &newEntity )
{
    if (auto l = std::dynamic_pointer_cast<insight::cad::Line>(newEntity))
    {
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(line_) == entity)
        {
            line_ = l;
        }
    }
}




void IQVTKHorizontalConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const IQVTKHorizontalConstraint&>(other));
}




ConstrainedSketchEntityPtr IQVTKHorizontalConstraint::clone() const
{
    auto cl=IQVTKHorizontalConstraint::create( line_, layerName() );

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef() = parameters();
    return cl;
}




void IQVTKHorizontalConstraint::operator=(const IQVTKHorizontalConstraint& other)
{
    line_=other.line_;
    IQVTKConstrainedSketchEntity::operator=(other);
}
