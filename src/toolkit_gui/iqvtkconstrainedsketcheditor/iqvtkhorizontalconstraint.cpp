#include "iqvtkhorizontalconstraint.h"

#include "constrainedsketch.h"
#include "constrainedsketchgrammar.h"

#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"


using namespace insight::cad;


defineType(IQVTKHorizontalConstraint);


IQVTKHorizontalConstraint::IQVTKHorizontalConstraint(
    std::shared_ptr<insight::cad::Line> line )
    : line_(line)
{}


std::vector<vtkSmartPointer<vtkProp> >
IQVTKHorizontalConstraint::createActor() const
{
    auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
    caption->SetCaption("H");
    caption->SetAttachmentPoint(
        arma::mat(0.5*
                  (line_->getDatumPoint("p0")+line_->getDatumPoint("p1")))
            .memptr());
    caption->GetTextActor()->SetTextScaleModeToNone(); //key: fix the font size
    caption->GetCaptionTextProperty()->SetColor(0,0,0);
    caption->GetCaptionTextProperty()->SetFontSize(10);
    caption->GetCaptionTextProperty()->SetFrame(false);
    caption->GetCaptionTextProperty()->SetShadow(false);

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
             > ruleset.r_parameters >
             ')' )
            [ std::cout<<"Reading "<< typeName <<" "<<qi::_1<<std::endl,
              qi::_val = phx::bind(
                 &IQVTKHorizontalConstraint::create<std::shared_ptr<insight::cad::Line> >,
                 phx::bind(&insight::cad::ConstrainedSketchGrammar::lookupEntity<insight::cad::Line>, phx::ref(ruleset), qi::_2) ),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_val, qi::_3, "."),
                 phx::insert(
                    phx::ref(ruleset.labeledEntities),
                    phx::construct<ConstrainedSketchGrammar::LabeledEntitiesMap::value_type>(qi::_1, qi::_val)) ]
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
