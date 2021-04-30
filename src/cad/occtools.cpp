/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  hannes <email>
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
 *
 */

#include "base/linearalgebra.h"
#include "occtools.h"
#include "base/units.h"
#include "Prs3d_Text.hxx"
#include "StdPrs_Point.hxx"
#if ((OCC_VERSION_MAJOR<7)&&(OCC_VERSION_MINOR<6))
#include "Graphic2d_Text.hxx"
#endif
#include "Select3D_SensitivePoint.hxx"
#include "Font_NameOfFont.hxx"
#include "Graphic3d_AspectText3d.hxx"
#include "Font_FontAspect.hxx"
#if ((OCC_VERSION_MAJOR<7)&&(OCC_VERSION_MINOR<=6))
#include "Prs3d_TextAspect.hxx"
#include "Prs3d_PointAspect.hxx"
#include "SelectMgr_Selection.hxx"
#include "SelectMgr_EntityOwner.hxx"
#endif

namespace insight {
namespace cad {



gp_Trsf OFtransformToOCC(const arma::mat &translate, const arma::mat &rollPitchYaw, double scale)
{
  gp_Trsf tr; tr.SetTranslation(to_Vec(translate));
  gp_Trsf rx; rx.SetRotation(gp::OX(), rollPitchYaw(0)*SI::deg);
  gp_Trsf ry; ry.SetRotation(gp::OY(), rollPitchYaw(1)*SI::deg);
  gp_Trsf rz; rz.SetRotation(gp::OZ(), rollPitchYaw(2)*SI::deg);
  gp_Trsf sc; sc.SetScaleFactor(scale);
  return sc*rz*ry*rx*tr;
}

OCCtransformToOF::OCCtransformToOF(const gp_Trsf &t)
{
  arma::mat R(3,3);
  for (int i=0;i<3;i++)
    for (int j=0;j<3;j++)
      R(i,j)=t.Value(i+1,j+1);

  scale_ = t.ScaleFactor();
  rollPitchYaw_ = rotationMatrixToRollPitchYaw(R);
  translate_ = (1./scale_)*inv(R)*Vector(t.TranslationPart()).t();
}



OCCtransformToVTK::OCCtransformToVTK(const gp_Trsf& t)
  : OCCtransformToOF(t)
{}

vtkSmartPointer<vtkTransform> OCCtransformToVTK::operator()() const
{
  auto t = vtkSmartPointer<vtkTransform>::New();
  t->Translate( translate()(0), translate()(1), translate()(2) );
  t->RotateX( rollPitchYaw()(0) );
  t->RotateY( rollPitchYaw()(1) );
  t->RotateZ( rollPitchYaw()(2) );
  t->Scale( scale(), scale(), scale() );
  return t;
}


Handle_AIS_MultipleConnectedInteractive
buildMultipleConnectedInteractive
(
    AIS_InteractiveContext &context,
    std::vector<Handle_AIS_InteractiveObject> objs
)
{
  Handle_AIS_MultipleConnectedInteractive ais ( new AIS_MultipleConnectedInteractive() );

#if (OCC_VERSION_MAJOR<=7 && OCC_VERSION_MINOR<4)
  context.Load(ais);
#endif

  for (auto o: objs)
  {
#if (OCC_VERSION_MAJOR<=7 && OCC_VERSION_MINOR<4)
    context.Load(o);
#endif
    ais->Connect(o);
  }

  return ais;
}

  
Handle_AIS_InteractiveObject createArrow(const TopoDS_Shape& shape, const std::string& text)
{
  Handle(AIS_RadiusDimension) dim=new AIS_RadiusDimension
  (
   shape
#if ((OCC_VERSION_MAJOR<7)&&(OCC_VERSION_MINOR<=6))
   , 1e-6, text.c_str()
  );
#else
  );
  dim->SetModelUnits(text.c_str());
#endif
//   Handle_Prs3d_TextAspect ta=dim->Attributes()->TextAspect();
//   ta->SetHeight(100.0);
//   dim->Attributes()->SetTextAspect(ta);
  return dim;
}

Handle_AIS_InteractiveObject createLengthDimension
(
  const TopoDS_Vertex& from, 
  const TopoDS_Vertex& to, 
  const Handle_Geom_Plane& pln,
  double L,
  const std::string& text
)
{
  Handle(AIS_LengthDimension) dim(new AIS_LengthDimension(
    from,
    to,
#if ((OCC_VERSION_MAJOR<7)&&(OCC_VERSION_MINOR<=6))
    pln,
    L, 
    text.c_str()
  ));
#else
    pln->Pln()
  ));
  if (text!="") dim->SetDisplayUnits(text.c_str());
#endif
  return dim;
}



IMPLEMENT_STANDARD_HANDLE (InteractiveText, AIS_InteractiveObject)
// IMPLEMENT_STANDARD_RTTI (InteractiveText, AIS_InteractiveObject)
#if (OCC_VERSION_MAJOR>=7)
IMPLEMENT_STANDARD_RTTIEXT (InteractiveText,AIS_InteractiveObject)
#else
IMPLEMENT_STANDARD_RTTI (InteractiveText)
#endif
//
// Foreach ancestors, we add a IMPLEMENT_STANDARD_SUPERTYPE and
// a IMPLEMENT_STANDARD_SUPERTYPE_ARRAY_ENTRY macro.
// We must respect the order: from the direct ancestor class
// to the base class.
//
IMPLEMENT_STANDARD_TYPE (InteractiveText)
IMPLEMENT_STANDARD_SUPERTYPE (AIS_InteractiveObject)
IMPLEMENT_STANDARD_SUPERTYPE (SelectMgr_SelectableObject)
IMPLEMENT_STANDARD_SUPERTYPE (PrsMgr_PresentableObject)
IMPLEMENT_STANDARD_SUPERTYPE (MMgt_TShared)
IMPLEMENT_STANDARD_SUPERTYPE (Standard_Transient)
IMPLEMENT_STANDARD_SUPERTYPE_ARRAY ()
// IMPLEMENT_STANDARD_SUPERTYPE_ARRAY_ENTRY (AIS_InteractiveObject)
// IMPLEMENT_STANDARD_SUPERTYPE_ARRAY_ENTRY (SelectMgr_SelectableObject)
// IMPLEMENT_STANDARD_SUPERTYPE_ARRAY_ENTRY (PrsMgr_PresentableObject)
// IMPLEMENT_STANDARD_SUPERTYPE_ARRAY_ENTRY (MMgt_TShared)
// IMPLEMENT_STANDARD_SUPERTYPE_ARRAY_ENTRY (Standard_Transient)
IMPLEMENT_STANDARD_SUPERTYPE_ARRAY_END ()
IMPLEMENT_STANDARD_TYPE_END (InteractiveText)



// ------------------- Initialization and Deletion ---------------------
InteractiveText::InteractiveText () :
    AIS_InteractiveObject (),
    text_("?"),
    width_ (0), 
    height_ (0)
{
}

InteractiveText::InteractiveText 
(
  const std::string& text, const arma::mat& pos,
  double angle, double slant,
  int color_id, int font_id,
  double scale
) 
: AIS_InteractiveObject (),
  text_ (text), 
  position_ (pos),
  angle_ (angle), 
  slant_ (slant),
  color_id_ (color_id), 
  font_id_ (font_id),
  scale_ (scale),
  width_ (0), 
  height_ (0)
{
//   Handle_Prs3d_TextAspect aTextAspect( new Prs3d_TextAspect());
//   aTextAspect->SetFont(Font_NOF_ASCII_MONO);
//   aTextAspect->SetHeight(12);
//   aTextAspect->SetColor(Quantity_NOC_BLUE1);
//   aTextAspect->SetSpace(200);
//   myDrawer->SetTextAspect (aTextAspect);
  


 SetDisplayMode (0);
}

InteractiveText::~InteractiveText ()
{
}

// -------------------------- Element change ---------------------------

void InteractiveText::set_text (const std::string& v)
{
    text_ = v;
}

void InteractiveText::set_position (const arma::mat& pos)
{
    position_ = pos;
}

// -------------------------- Implementation ---------------------------

void InteractiveText::Compute (const Handle_PrsMgr_PresentationManager3d& /*pm*/,
                               const Handle(Prs3d_Presentation)& pres,
                               const Standard_Integer mode)
{
  Handle_Prs3d_TextAspect at=new Prs3d_TextAspect();
  at->SetColor(Quantity_NOC_BLACK);
  myDrawer->SetTextAspect (at);

  TCollection_ExtendedString aTCoText(text_.c_str());
  gp_Pnt location(position_(0), position_(1), position_(2));
  
  Prs3d_Text::Draw 
  (
      pres, 
      myDrawer,
      aTCoText,
      location
  );
  
  myDrawer->PointAspect()->SetColor(Quantity_NOC_BLACK);
  Handle_Geom_Point p(new Geom_CartesianPoint(location));
  StdPrs_Point::Add(pres,p,myDrawer);

}

// void InteractiveText::Compute (const Handle_Prs3d_Projector& proj,
//                                const Handle_Prs3d_Presentation& pres)
// {
// 
// }

#if ((OCC_VERSION_MAJOR<7)&&(OCC_VERSION_MINOR<6))
void InteractiveText::Compute (const Handle_PrsMgr_PresentationManager2d& pres,
                               const Handle_Graphic2d_GraphicObject& gr_obj,
                               const Standard_Integer mode)
{
    Handle_Graphic2d_Text text =
        new Graphic2d_Text (gr_obj, (Standard_CString) text_.c_str(),
                            position_(0), position_(1),
                            angle_, Aspect_TOT_SOLID, scale_);
    text->SetFontIndex (font_id_);
    text->SetColorIndex (color_id_);

    text->SetSlant (slant_);
    text->SetUnderline (false);
    text->SetZoomable (true);
    gr_obj->Display ();
    double x_offset;
    double y_offset;
    text->TextSize (width_, height_, x_offset, y_offset);
}
#endif

void InteractiveText::ComputeSelection (const Handle_SelectMgr_Selection& sel,
                                        const Standard_Integer mode)
{
  Handle(SelectMgr_EntityOwner)   anEntityOwner   = new SelectMgr_EntityOwner (this, 10);
      Handle(Select3D_SensitivePoint) aSensitivePoint = new Select3D_SensitivePoint (anEntityOwner, gp_Pnt (position_(0),
                              position_(1),
                              position_(2)));
      sel->Add (aSensitivePoint);
}

}
}
