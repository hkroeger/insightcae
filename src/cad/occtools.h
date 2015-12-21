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

#ifndef INSIGHT_OCCTOOLS_H
#define INSIGHT_OCCTOOLS_H

#include "occinclude.h"

namespace insight {
namespace cad {
  
Handle_AIS_InteractiveObject createArrow(const TopoDS_Shape& shape, const std::string& text);
Handle_AIS_InteractiveObject createLengthDimension
(
  const TopoDS_Vertex& from, 
  const TopoDS_Vertex& to, 
  const Handle_Geom_Plane& pln,
  double L,
  const std::string& text
);


/*! \class InteractiveText
* \brief Interactive items specialized in the displaying of texts.
*/

DEFINE_STANDARD_HANDLE (InteractiveText, AIS_InteractiveObject)

class InteractiveText 
: public AIS_InteractiveObject
{
protected:
    std::string text_;
    arma::mat	position_;
    double	angle_;
    double	slant_;
    int		color_id_;
    int		font_id_;
    double	scale_;

    double	width_;
    double	height_;

// ------------------- Initialization and Deletion ---------------------
public:
    //! Construct a default instance of InteractiveText.
    InteractiveText ();

    //! Construct a fully initialized instance of InteractiveText.
    InteractiveText 
    (
      const std::string& text, const arma::mat& pos,
      double angle = 0, double slant = 0,
      int color_id = 1, int font_id = 1,
      double scale = 0.2
    );

    //! Destruct the instance and free any allocated resources.
    virtual ~InteractiveText ();

    DEFINE_STANDARD_RTTI (InteractiveText)

// ----------------------------- Access --------------------------------
public:
    //! Position of the text displayed.
    const arma::mat& position () const
    {
        return position_;
    }

    //! Text displated by the current InteractiveText object.
    const std::string& text () const
    {
        return text_;
    }

//     //! User friendly string of the instance.
//     virtual std::string& to_string () const
//     {
//         return text_;
//     }

// -------------------------- Element change ---------------------------
public:
    void set_text (const std::string& v);

    void set_position (const arma::mat& pos);

// -------------------------- Implementation ---------------------------
private:
//! -- from PrsMgr_PresentableObject.
    void Compute (const Handle_PrsMgr_PresentationManager3d& pm,
                  const Handle_Prs3d_Presentation& pres,
                  const Standard_Integer mode);

//! -- from PrsMgr_PresentableObject.
    void Compute (const Handle_Prs3d_Projector& proj,
                  const Handle_Prs3d_Presentation& pres);

//! -- from PrsMgr_PresentableObject.
    void Compute (const Handle_PrsMgr_PresentationManager2d& pres,
                  const Handle_Graphic2d_GraphicObject& gr_obj,
                  const Standard_Integer mode) ;

//! -- from SelectMgr_SelectableObject.
    void ComputeSelection (const Handle_SelectMgr_Selection& sel,
                           const Standard_Integer mode);

// ---------------------------- Attributes -----------------------------

};

}
}

#endif // INSIGHT_OCCTOOLS_H
