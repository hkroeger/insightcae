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

#include "cadfeature.h"
#include "pointdistance.h"

#include "vtkActor.h"
#include "vtkDoubleArray.h"
#include "vtkLineSource.h"
#include "vtkPolyLine.h"
#include "vtkPolyLineSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkStringArray.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkLabelPlacementMapper.h"
#include "vtkTextProperty.h"
#include "vtkGlyph3D.h"
#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"

namespace insight {
namespace cad {


defineType(Distance);



size_t Distance::calcHash() const
{
  ParameterListHash h;
  h+=p1_->value();
  h+=p2_->value();
  return h.getHash();
}

arma::mat Distance::measureDirection() const
{
    insight::CurrentExceptionContext ex("determining direction of distance measurement");

    arma::mat delta=vec3Zero();
    if (distanceAlong_)
    {
        delta= distanceAlong_->value();
    }
    else
    {
        delta= p2_->value() - p1_->value();
    }

    if (arma::norm(delta, 2)<insight::SMALL)
        return vec3X(1);
    else
        return normalized( delta );
}


double Distance::calcDistance() const
{
    arma::mat p1=p1_->value();
    arma::mat p2=p2_->value();

    arma::mat d = p2 - p1;

    return arma::dot(d, measureDirection());
}


Distance::Distance(
    insight::cad::VectorPtr p1, insight::cad::VectorPtr p2,
    VectorPtr distanceAlong )
  : p1_(p1), p2_(p2), distanceAlong_(distanceAlong)
{}




void insight::cad::Distance::build()
{
  distance_ = calcDistance();
}




void insight::cad::Distance::write(ostream&) const
{}




void Distance::operator=(const Distance &other)
{
  p1_=other.p1_;
  p2_=other.p2_;
  distanceAlong_=other.distanceAlong_;
  distance_=other.distance_;
  PostprocAction::operator=(other);
}




arma::mat Distance::dimLineOffset() const
{
  insight::CurrentExceptionContext ex("determining dimension line offset direction");

  arma::mat dir = measureDirection();
  arma::mat n = arma::cross(dir, vec3(0,0,1));
  if (arma::norm(n,2)<SMALL)
      n=arma::cross(dir, vec3(0,1,0));
  return normalized(n)*0.025*arma::norm(dir,2);
}




double Distance::relativeArrowSize() const
{
    return 0.025;
}

double Distance::distance() const
{
    checkForBuildDuringAccess();
    return distance_;
}


std::vector<vtkSmartPointer<vtkProp> >
Distance::createVTKRepr(bool displayCoords) const
{
    auto dist = this;
    insight::assertion( bool(dist), "internal error: expected distance object") ;

    arma::mat dir = dist->measureDirection();
    double L=dist->distance();
    arma::mat p1 = dist->p1_->value();
    arma::mat p2 = p1 + dir*L; //dist->p2_->value();

    arma::mat ofs = dist->dimLineOffset();
    arma::mat pmid = 0.5*(p1+p2);

    double relArrSize = 2.*dist->relativeArrowSize();

    vtkNew<vtkPolyData> a0;
    {
        // Create a vtkPoints object and store the points in it
        vtkNew<vtkPoints> points;
        points->InsertNextPoint(0,0,0);
        points->InsertNextPoint(1.-relArrSize, 0, 0);
        points->InsertNextPoint(1.-relArrSize, relArrSize/5., 0);
        points->InsertNextPoint(1,0,0);
        points->InsertNextPoint(1.-relArrSize, -relArrSize/5., 0);
        points->InsertNextPoint(1.-relArrSize, 0, 0);

        vtkNew<vtkPolyLine> polyLine;
        polyLine->GetPointIds()->SetNumberOfIds(points->GetNumberOfPoints());
        for (unsigned int i = 0; i < 6; i++)
        {
            polyLine->GetPointIds()->SetId(i, i);
        }

        // Create a cell array to store the lines in and add the lines to it
        vtkNew<vtkCellArray> cells;
        cells->InsertNextCell(polyLine);

        // Create a polydata to store everything in

        // Add the points to the dataset
        a0->SetPoints(points);

        // Add the lines to the dataset
        a0->SetLines(cells);
    }


    //auto tf= vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    auto gdpts = vtkSmartPointer<vtkPoints>::New();
    gdpts->SetNumberOfPoints(2);

    auto gddirs = vtkSmartPointer<vtkDoubleArray>::New();
    gddirs->SetNumberOfComponents(3);
    gddirs->SetNumberOfTuples(2);
    gddirs->SetName("dir");

    // set data
    gdpts->SetPoint(0, arma::mat(pmid+ofs).memptr());
    gddirs->SetTypedTuple(
        0, arma::mat(p1-pmid).memptr() );

    gdpts->SetPoint(1, arma::mat(pmid+ofs).memptr());
    gddirs->SetTypedTuple(
        1, arma::mat(p2-pmid).memptr() );

    auto gd = vtkSmartPointer<vtkPolyData>::New();
    gd->SetPoints(gdpts);
    gd->GetPointData()->AddArray(gddirs);
    gd->GetPointData()->SetActiveVectors("dir");


    auto gl = vtkSmartPointer<vtkGlyph3D>::New();
    gl->SetInputData(gd); //data
    gl->SetSourceData(a0); //arrow
    gl->SetVectorModeToUseVector();
    gl->SetScaleModeToScaleByVector();
    gl->SetInputArrayToProcess(
        1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "dir");



    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    //mapper->SetInputData(arr);
    mapper->SetInputConnection(gl->GetOutputPort());
    mapper->Update();
    auto arrAct = vtkSmartPointer<vtkActor>::New();
    arrAct->SetMapper( mapper );
    arrAct->GetProperty()->SetColor(0.5, 0.5, 0.5);

    auto hl1 = vtkSmartPointer<vtkLineSource>::New();
    hl1->SetPoint1(p1.memptr());
    hl1->SetPoint2(arma::mat(p1+ofs).memptr());
    auto mapper1 = vtkSmartPointer<vtkPolyDataMapper>::New();
    //mapper->SetInputData(arr);
    mapper1->SetInputConnection(hl1->GetOutputPort());
    mapper1->Update();
    auto arrAct1 = vtkSmartPointer<vtkActor>::New();
    arrAct1->SetMapper( mapper1 );
    arrAct1->GetProperty()->SetColor(0.5, 0.5, 0.5);

    auto hl2 = vtkSmartPointer<vtkLineSource>::New();
    hl2->SetPoint1(dist->p2_->value().memptr());
    hl2->SetPoint2(arma::mat(p2+ofs).memptr());
    auto mapper2 = vtkSmartPointer<vtkPolyDataMapper>::New();
    //mapper->SetInputData(arr);
    mapper2->SetInputConnection(hl2->GetOutputPort());
    mapper2->Update();
    auto arrAct2 = vtkSmartPointer<vtkActor>::New();
    arrAct2->SetMapper( mapper2 );
    arrAct2->GetProperty()->SetColor(0.5, 0.5, 0.5);


    auto lblActor = vtkSmartPointer<vtkActor2D>::New();

    if (displayCoords)
    {
        int nl=displayCoords?3:1;
        auto points = vtkSmartPointer<vtkPoints>::New();
        points->SetNumberOfPoints(nl);
        auto labels = vtkSmartPointer<vtkStringArray>::New();
        labels->SetName("labels");
        labels->SetNumberOfValues(nl);
        auto sizes = vtkSmartPointer<vtkIntArray>::New();
        sizes->SetName("sizes");
        sizes->SetNumberOfValues(nl);

        int il=0;
        points->SetPoint(il, arma::mat(pmid+ofs).memptr());
        labels->SetValue(il, str(boost::format("L=%g") % L ).c_str());
        sizes->SetValue(il, 6);
        il++;

        if (displayCoords)
        {
            points->SetPoint(il, arma::mat(p1+ofs).memptr());
            labels->SetValue(il, str(boost::format("[%g %g %g]") % p1(0)%p1(1)%p1(2) ).c_str());
            sizes->SetValue(il, 4);
            il++;

            points->SetPoint(il, arma::mat(p2+ofs).memptr());
            labels->SetValue(il, str(boost::format("[%g %g %g]") % p2(0)%p2(1)%p2(2) ).c_str());
            sizes->SetValue(il, 4);
            il++;
        }

        auto pointSource = vtkSmartPointer<vtkPolyData>::New();
        pointSource->SetPoints(points);
        pointSource->GetPointData()->AddArray(labels);
        pointSource->GetPointData()->AddArray(sizes);

        auto pts2Lbl = vtkSmartPointer<vtkPointSetToLabelHierarchy>::New();
        pts2Lbl->SetInputData(pointSource);
        pts2Lbl->SetLabelArrayName("labels");
        pts2Lbl->SetPriorityArrayName("sizes");
        pts2Lbl->GetTextProperty()->SetColor(0,0,0);
        pts2Lbl->Update();

        // Create a mapper and actor for the labels.
        auto lblMap = vtkSmartPointer<vtkLabelPlacementMapper>::New();
        lblMap->SetInputConnection(
            pts2Lbl->GetOutputPort());

        lblActor->SetMapper(lblMap);
    }
    else
    {
        auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
        caption->BorderOff();
        caption->SetCaption(str(boost::format("%g") % L ).c_str());
        caption->SetAttachmentPoint( arma::mat(pmid+ofs).memptr() );
        caption->GetTextActor()->SetTextScaleModeToNone(); //key: fix the font size
        caption->GetCaptionTextProperty()->SetColor(0,0,0);
        caption->GetCaptionTextProperty()->SetJustificationToCentered();
        caption->GetCaptionTextProperty()->SetFontSize(10);
        caption->GetCaptionTextProperty()->FrameOff();
        caption->GetCaptionTextProperty()->ShadowOff();
        caption->GetCaptionTextProperty()->BoldOff();

        lblActor=caption;
    }

    arrAct->GetProperty()->SetLineWidth(0.5);
    arrAct1->GetProperty()->SetLineWidth(0.5);
    arrAct2->GetProperty()->SetLineWidth(0.5);

    return { lblActor, arrAct, arrAct1, arrAct2 };
}

std::vector<vtkSmartPointer<vtkProp> >
Distance::createVTKRepr() const
{
    return createVTKRepr(true);
}

}
}
