#include "iqvtkpointoncurveconstraint.h"

#include "cadfeatures/line.h"

#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"


defineType(IQVTKPointOnCurveConstraint);

IQVTKPointOnCurveConstraint::IQVTKPointOnCurveConstraint(
    std::shared_ptr<insight::cad::SketchPoint> p,
    std::shared_ptr<insight::cad::Feature> curve )
    : p_(p),
    curve_(curve)
{}

std::vector<vtkSmartPointer<vtkProp> >
IQVTKPointOnCurveConstraint::createActor() const
{
    auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
    caption->SetCaption("F");
    caption->SetAttachmentPoint(
        arma::mat(p_->value()).memptr()
        );
    caption->GetTextActor()->SetTextScaleModeToNone(); //key: fix the font size
    caption->GetCaptionTextProperty()->SetColor(0,0,0);
    caption->GetCaptionTextProperty()->SetFontSize(10);
    caption->GetCaptionTextProperty()->SetFrame(false);
    caption->GetCaptionTextProperty()->SetShadow(false);

    return {caption};
}

int IQVTKPointOnCurveConstraint::nConstraints() const
{
    return 1;
}

double IQVTKPointOnCurveConstraint::getConstraintError(unsigned int iConstraint) const
{
    insight::assertion(
        iConstraint==0,
        "invalid constraint index");

    if (auto line = std::dynamic_pointer_cast<insight::cad::Line>(curve_))
    {
        arma::mat a = line->start()->value();
        arma::mat b = insight::normalized(line->end()->value() - a);
        arma::mat p = p_->value();
        double nom=arma::norm( arma::cross(p-a, b), 2 );
        double denom=arma::norm(b, 2);
        if (fabs(denom)<insight::SMALL)
            return DBL_MAX;
        else
            return nom/denom;
    }
    else
    {
        return curve_->minDist(p_->value());
    }
}

void IQVTKPointOnCurveConstraint::scaleSketch(double scaleFactor)
{
}

void IQVTKPointOnCurveConstraint::generateScriptCommand(
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
            + parameterString()
            + ")"
        );
}

namespace insight { namespace cad {
addToStaticFunctionTable(ConstrainedSketchEntity, IQVTKPointOnCurveConstraint, addParserRule);
}}

void IQVTKPointOnCurveConstraint::addParserRule(
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
             > qi::int_ > ','
             > qi::int_
             > ruleset.r_parameters >
             ')' )
                [ qi::_val = phx::bind(
                 &IQVTKPointOnCurveConstraint::create<insight::cad::SketchPointPtr, insight::cad::FeaturePtr>,
                    phx::bind(&insight::cad::ConstrainedSketchGrammar::lookupEntity<insight::cad::SketchPoint>, phx::ref(ruleset), qi::_3),
                    phx::bind(&insight::cad::ConstrainedSketchGrammar::lookupEntity<insight::cad::Line>, phx::ref(ruleset), qi::_2)
                    ),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_val, qi::_4, "."),
                 phx::insert(
                    phx::ref(ruleset.labeledEntities),
                    phx::construct<ConstrainedSketchGrammar::LabeledEntitiesMap::value_type>(qi::_1, qi::_val)) ]
            );
}

std::set<std::comparable_weak_ptr<insight::cad::ConstrainedSketchEntity> >
IQVTKPointOnCurveConstraint::dependencies() const
{
    std::set<std::comparable_weak_ptr<insight::cad::ConstrainedSketchEntity> > ret
        { p_ };

    if (auto l = std::dynamic_pointer_cast<insight::cad::ConstrainedSketchEntity>(curve_))
        ret.insert(l);

    return ret;
}

void IQVTKPointOnCurveConstraint::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity> &entity,
    const std::shared_ptr<ConstrainedSketchEntity> &newEntity)
{
    if (auto p = std::dynamic_pointer_cast<insight::cad::SketchPoint>(newEntity))
    {
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(p_) == entity)
        {
            p_ = p;
        }
    }
    if (auto c = std::dynamic_pointer_cast<insight::cad::Feature>(newEntity))
    {
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(curve_) == entity)
        {
            curve_ = c;
        }
    }
}
