#include "sketchpoint.h"

#include "datum.h"
#include "constrainedsketch.h"

#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointSource.h>

namespace insight {
namespace cad {


defineType(SketchPoint);
addToStaticFunctionTable(ConstrainedSketchEntity, SketchPoint, addParserRule);


SketchPoint::SketchPoint(const SketchPoint &o, TreeCloneMap &tcm)
    : CL(plane_), x_(o.x_), y_(o.y_)
{}

SketchPoint::SketchPoint(
    DatumPtr plane,
    const arma::mat& xy,
    const std::string& layerName )
  : ConstrainedSketchEntity(layerName),
    plane_(plane),
    x_(xy(0)), y_(xy(1))
{
}

SketchPoint::SketchPoint(
    DatumPtr plane,
    double x, double y,
    const std::string& layerName )
  : ConstrainedSketchEntity(layerName),
    plane_(plane),
    x_(x), y_(y)
{}

void SketchPoint::setCoords2D(double x, double y)
{
    x_=x;
    y_=y;
}

void SketchPoint::setCoords2D(const arma::mat& xy)
{
    setCoords2D(xy(0), xy(1));
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
            .Translated(pl.XDirection().XYZ()*coords2D()(0))
            .Translated(pl.YDirection().XYZ()*coords2D()(1))
        );
}

DatumPtr SketchPoint::plane() const
{
    return plane_;
}

int SketchPoint::nDoF() const
{
    return 2;
}


double SketchPoint::getDoFValue(unsigned int iDoF) const
{
    switch (iDoF)
    {
    case 0: return coords2D()(0); break;
    case 1: return coords2D()(1); break;
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
    case 0: setCoords2D(value, coords2D()(1)); break;
    case 1: setCoords2D(coords2D()(0), value); break;
    default:
        throw insight::Exception(
            "invalid DoF index: %d", iDoF );

    }
}

void SketchPoint::scaleSketch(double scaleFactor)
{
    setCoords2D(
        scaleFactor*coords2D()(0),
        scaleFactor*coords2D()(1)
    );
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
            + ", layer " + layerName()
            + parameterString()
            + ")"
        );
}

void SketchPoint::addParserRule(
    ConstrainedSketchGrammar& ruleset,
    const ConstrainedSketchParametersDelegate& pd )
{
    namespace qi=boost::spirit::qi;
    namespace phx=boost::phoenix;
    ruleset.entityRules.add
        (
            typeName,
            ( '('
             > qi::int_ > ','
             > qi::double_ > ',' > qi::double_
             > (( ',' >> qi::lit("layer") >> ruleset.r_label) | qi::attr(std::string()))
             > ruleset.r_parameters >
             ')' )
            [ qi::_a = phx::bind(
                 &SketchPoint::create<DatumPtr, double, double, const std::string&>,
                   ruleset.sketch->plane(), qi::_2, qi::_3, qi::_4),
                 phx::bind(&ConstrainedSketchParametersDelegate::changeDefaultParameters, &pd, phx::ref(*qi::_a)),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_a, qi::_5,
                    boost::filesystem::path(".")),
                 qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(qi::_1, qi::_a) ]
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

bool SketchPoint::isInside(SelectionRect r) const
{
    return r.isInside(value());
}


void SketchPoint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const SketchPoint&>(other));
}

ConstrainedSketchEntityPtr SketchPoint::clone() const
{
    auto cl=SketchPoint::create(
        plane_,
        coords2D(),
        layerName() );

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef() = parameters();
    return cl;
}

std::vector<vtkSmartPointer<vtkProp> > SketchPoint::createActor() const
{
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );

    auto point = vtkSmartPointer<vtkPointSource>::New();
    auto p = value();
    point->SetCenter(p[0], p[1], p[2]);
    point->SetNumberOfPoints(1);
    point->SetRadius(0);
    actor->GetMapper()->SetInputConnection(point->GetOutputPort());
    auto prop=actor->GetProperty();
    prop->SetRepresentationToPoints();
    prop->SetPointSize(8);
    prop->SetColor(0, 0, 0);

    return {actor};
}


void SketchPoint::operator=(const SketchPoint& other)
{
    plane_=other.plane_;
    x_=other.x_;
    y_=other.y_;
    Vector::operator=(other);
    ConstrainedSketchEntity::operator=(other);
}


} // namespace cad
} // namespace insight
