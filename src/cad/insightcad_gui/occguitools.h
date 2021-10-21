#ifndef OCCGUITOOLS_H
#define OCCGUITOOLS_H

#include "occinclude.h"

namespace insight {
namespace cad {

Handle_AIS_MultipleConnectedInteractive
buildMultipleConnectedInteractive
(
    AIS_InteractiveContext& context,
    std::vector<Handle_AIS_InteractiveObject> objs
);


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

#if (OCC_VERSION_MAJOR>=7)
    DEFINE_STANDARD_RTTIEXT (InteractiveText,AIS_InteractiveObject);
#else
    DEFINE_STANDARD_RTTI (InteractiveText);
#endif

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
                  const Handle(Prs3d_Presentation)& pres,
                  const Standard_Integer mode);

//! -- from PrsMgr_PresentableObject.
//     void Compute (const Handle_Prs3d_Projector& proj,
//                   const Handle_Prs3d_Presentation& pres);
#if ((OCC_VERSION_MAJOR<7)&&(OCC_VERSION_MINOR<6))
//! -- from PrsMgr_PresentableObject.
    void Compute (const Handle_PrsMgr_PresentationManager2d& pres,
                  const Handle_Graphic2d_GraphicObject& gr_obj,
                  const Standard_Integer mode) ;
#endif
//! -- from SelectMgr_SelectableObject.
    void ComputeSelection (const Handle_SelectMgr_Selection& sel,
                           const Standard_Integer mode);

// ---------------------------- Attributes -----------------------------

};

void ActivateAll(Handle_AIS_InteractiveContext context, TopAbs_ShapeEnum mode);
void DeactivateAll(Handle_AIS_InteractiveContext context, TopAbs_ShapeEnum mode);

}
}

#endif // OCCGUITOOLS_H
