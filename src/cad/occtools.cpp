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

#include "occtools.h"

#include "Prs3d_Text.hxx"
#if (OCC_VERSION_MINOR<7)
#include "Graphic2d_Text.hxx"
#endif

namespace insight {
namespace cad {
  
Handle_AIS_InteractiveObject createArrow(const TopoDS_Shape& shape, const std::string& text)
{
  Handle_AIS_RadiusDimension dim=new AIS_RadiusDimension
  (
   shape
#if (OCC_VERSION_MINOR<7)
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
  Handle_AIS_LengthDimension dim(new AIS_LengthDimension(
    from,
    to,
#if (OCC_VERSION_MINOR<7)
    pln,
    L, 
    text.c_str()
  ));
#else
    pln->Pln()
  ));
  dim->SetDisplayUnits(text.c_str());
#endif
  return dim;
}



IMPLEMENT_STANDARD_HANDLE (InteractiveText, AIS_InteractiveObject)
// IMPLEMENT_STANDARD_RTTI (InteractiveText, AIS_InteractiveObject)
IMPLEMENT_STANDARD_RTTI (InteractiveText)
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
IMPLEMENT_STANDARD_SUPERTYPE_ARRAY_ENTRY (AIS_InteractiveObject)
IMPLEMENT_STANDARD_SUPERTYPE_ARRAY_ENTRY (SelectMgr_SelectableObject)
IMPLEMENT_STANDARD_SUPERTYPE_ARRAY_ENTRY (PrsMgr_PresentableObject)
IMPLEMENT_STANDARD_SUPERTYPE_ARRAY_ENTRY (MMgt_TShared)
IMPLEMENT_STANDARD_SUPERTYPE_ARRAY_ENTRY (Standard_Transient)
IMPLEMENT_STANDARD_SUPERTYPE_ARRAY_END ()
IMPLEMENT_STANDARD_TYPE_END (InteractiveText)



// ------------------- Initialization and Deletion ---------------------
InteractiveText::InteractiveText () :
    AIS_InteractiveObject (),
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

void InteractiveText::Compute (const Handle_PrsMgr_PresentationManager3d& pm,
                               const Handle_Prs3d_Presentation& pres,
                               const Standard_Integer mode)
{
    Prs3d_Text::Draw (pres, myDrawer, (Standard_CString) text_.c_str (),
                      gp_Pnt (position_(0),
                              position_(1),
                              position_(2)));
}

void InteractiveText::Compute (const Handle_Prs3d_Projector& proj,
                               const Handle_Prs3d_Presentation& pres)
{

}

#if (OCC_VERSION_MINOR<7)
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
}

}
}
