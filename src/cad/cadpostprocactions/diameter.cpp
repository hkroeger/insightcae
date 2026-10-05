/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "base/exception.h"
#include "cadfeature.h"
#include "cadparameters.h"
#include "datum.h"
#include "diameter.h"
#include "parser_tools.h"

#include "gce_MakeCirc.hxx"

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkLineSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegularPolygonSource.h"
#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

namespace insight {
namespace cad {


defineType(DiameterMeasurement);

addToStaticFunctionTable2(
    PostprocAction, InsertRule, insertrule,
    DiameterMeasurement, &DiameterMeasurement::insertrule );




DiameterMeasurement::DiameterMeasurement(FeatureSetPtr edge)
  : mode_(CircularEdge),
    edge_(edge)
{}




DiameterMeasurement::DiameterMeasurement(
    Mode mode, VectorPtr v1, VectorPtr v2, VectorPtr v3 )
  : mode_(mode),
    v1_(v1), v2_(v2), v3_(v3)
{
  insight::assertion(
      mode_!=CircularEdge,
      "internal error: circular edge mode requires an edge feature set" );
}




size_t DiameterMeasurement::calcHash() const
{
  ParameterListHash h;
  h+=int(mode_);
  if (edge_) h+=*edge_;
  if (v1_) h+=*v1_;
  if (v2_) h+=*v2_;
  if (v3_) h+=*v3_;
  return h.getHash();
}




void DiameterMeasurement::build()
{
  switch (mode_)
  {
    case CircularEdge:
    {
      double D;
      CircleEdgeCenterCoords(edge_).compute(center_, D, normal_);
      diameter_=D;

      // dimension line towards start point of edge
      FeatureID i=*(edge_->data().begin());
      double c0, c1;
      Handle_Geom_Curve crv(BRep_Tool::Curve(edge_->model()->edge(i), c0, c1));
      arma::mat d = vec3(crv->Value(c0)) - center_;
      d -= arma::dot(d, normal_)*normal_;
      if (arma::norm(d,2) > SMALL)
      {
        refDir_ = normalized(d);
      }
      else
      {
        arma::mat n = arma::cross(normal_, vec3X(1));
        if (arma::norm(n,2)<SMALL)
          n=arma::cross(normal_, vec3Y(1));
        refDir_ = normalized(n);
      }
      break;
    }

    case CenterNormalPoint:
    {
      center_ = v1_->value();
      arma::mat n = v2_->value();
      if (arma::norm(n,2)<SMALL)
        throw insight::Exception("normal vector must not be zero!");
      normal_ = normalized(n);

      arma::mat d = v3_->value() - center_;
      d -= arma::dot(d, normal_)*normal_;
      double r = arma::norm(d,2);
      if (r<SMALL)
        throw insight::Exception("point on curve must not coincide with the center point!");

      diameter_ = 2.*r;
      refDir_ = d/r;
      break;
    }

    case ThreePoints:
    {
      arma::mat p1=v1_->value(), p2=v2_->value(), p3=v3_->value();
      gce_MakeCirc mc(to_Pnt(p1), to_Pnt(p2), to_Pnt(p3));
      if (!mc.IsDone())
        throw insight::Exception("could not construct a circle through the three given points! (collinear or coincident points?)");

      gp_Circ c = mc.Value();
      center_ = vec3(c.Location());
      normal_ = normalized(vec3(c.Axis().Direction()));
      diameter_ = 2.*c.Radius();
      refDir_ = normalized(p1 - center_);
      break;
    }
  }
}




void DiameterMeasurement::write(std::ostream& console) const
{
  console << "diameter = " << diameter_ << std::endl;
  console << "center = [" << center_(0) << " " << center_(1) << " " << center_(2) << "]" << std::endl;
  console << "normal = [" << normal_(0) << " " << normal_(1) << " " << normal_(2) << "]" << std::endl;
}




double DiameterMeasurement::diameter() const
{
  checkForBuildDuringAccess();
  return diameter_;
}

arma::mat DiameterMeasurement::center() const
{
  checkForBuildDuringAccess();
  return center_;
}

arma::mat DiameterMeasurement::normal() const
{
  checkForBuildDuringAccess();
  return normal_;
}




std::vector<vtkSmartPointer<vtkProp> >
DiameterMeasurement::createVTKRepr() const
{
  checkForBuildDuringAccess();

  double r = 0.5*diameter_;
  arma::mat p1 = center_ - r*refDir_;
  arma::mat p2 = center_ + r*refDir_;

  std::vector<vtkSmartPointer<vtkProp> > actors;

  auto addActor = [&](vtkAlgorithm* src)
  {
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(src->GetOutputPort());
    auto act = vtkSmartPointer<vtkActor>::New();
    act->SetMapper(mapper);
    act->GetProperty()->SetColor(0.5, 0.5, 0.5);
    act->GetProperty()->SetLineWidth(0.5);
    actors.push_back(act);
  };

  // dimension line
  auto dl = vtkSmartPointer<vtkLineSource>::New();
  dl->SetPoint1(p1.memptr());
  dl->SetPoint2(p2.memptr());
  addActor(dl);

  // arrow heads, pointing outwards
  double h = 0.05*diameter_;
  for (const arma::mat& dir: { arma::mat(-refDir_), arma::mat(refDir_) })
  {
    arma::mat tip = center_ + r*dir;
    arma::mat ctr = tip - 0.5*h*dir;
    auto cone = vtkSmartPointer<vtkConeSource>::New();
    cone->SetCenter(ctr.memptr());
    cone->SetDirection(arma::mat(dir).memptr());
    cone->SetHeight(h);
    cone->SetRadius(0.2*h);
    cone->SetResolution(12);
    addActor(cone);
  }

  // circle outline (there is no model edge to look at in the point-based modes)
  if (mode_!=CircularEdge)
  {
    auto circ = vtkSmartPointer<vtkRegularPolygonSource>::New();
    circ->GeneratePolygonOff();
    circ->SetNumberOfSides(64);
    circ->SetCenter(center_.memptr());
    circ->SetNormal(arma::mat(normal_).memptr());
    circ->SetRadius(r);
    addActor(circ);
  }

  // label
  auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
  caption->BorderOff();
  caption->SetCaption(str(boost::format("Ø%g") % diameter_ ).c_str());
  caption->SetAttachmentPoint( arma::mat(center_+0.5*r*refDir_).memptr() );
  caption->GetTextActor()->SetTextScaleModeToNone(); //key: fix the font size
  caption->GetCaptionTextProperty()->SetColor(0,0,0);
  caption->GetCaptionTextProperty()->SetJustificationToCentered();
  caption->GetCaptionTextProperty()->SetFontSize(15);
  caption->GetCaptionTextProperty()->FrameOff();
  caption->GetCaptionTextProperty()->ShadowOff();
  caption->GetCaptionTextProperty()->BoldOff();
  actors.push_back(caption);

  return actors;
}




void DiameterMeasurement::insertrule(parser::ISCADParser& ruleset)
{
    ruleset.postProcFunctionRules.add
        (
            "Diameter",
            std::make_shared<parser::ISCADParser::PostProcFunctionRule>(
                ( '(' > ruleset.r_identifier > ','
                  > (
                      ( parser::kw("center")
                        > ruleset.r_vectorExpression > ','
                        > ruleset.r_vectorExpression > ','
                        > ruleset.r_vectorExpression )
                        [ qi::_val = phx::bind(
                             &DiameterMeasurement::create<Mode, VectorPtr, VectorPtr, VectorPtr>,
                             CenterNormalPoint, qi::_1, qi::_2, qi::_3) ]
                    |
                      ( parser::kw("points")
                        > ruleset.r_vectorExpression > ','
                        > ruleset.r_vectorExpression > ','
                        > ruleset.r_vectorExpression )
                        [ qi::_val = phx::bind(
                             &DiameterMeasurement::create<Mode, VectorPtr, VectorPtr, VectorPtr>,
                             ThreePoints, qi::_1, qi::_2, qi::_3) ]
                    |
                      ruleset.r_edgeFeaturesExpression
                        [ qi::_val = phx::bind(
                             &DiameterMeasurement::create<FeatureSetPtr>,
                             qi::_1) ]
                    )
                  > ')' > ';' )
                )
            );
}




}
}
