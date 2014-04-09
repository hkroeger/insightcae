#ifndef QOCC_H
#define QOCC_H

#include <AIS_StatusOfDetection.hxx>
#include <AIS_StatusOfPick.hxx>
#include <Aspect_Drawable.hxx>
#include <Aspect_GridDrawMode.hxx>
#include <Aspect_GridType.hxx>
#include <Aspect_GraphicCallbackProc.hxx>
#include <Handle_AIS_InteractiveContext.hxx>
#include <Handle_V3d_View.hxx>
#include <Handle_V3d_Viewer.hxx>
#include <Handle_TopTools_HSequenceOfShape.hxx>
#include <Standard_TypeDef.hxx>
#include <Quantity_Factor.hxx>
#include <Quantity_Length.hxx>
#include <Quantity_NameOfColor.hxx>
#include <V3d_Coordinate.hxx>

#ifdef WNT
#include <Handle_WNT_Window.hxx>
#else
#include <Handle_Xw_Window.hxx>
#endif

#ifdef QOCC_STATIC
#define QOCC_DECLSPEC
#else
#ifdef QOCC_MAKEDLL
#define QOCC_DECLSPEC Q_DECL_EXPORT
#else
#define QOCC_DECLSPEC Q_DECL_IMPORT
#endif
#endif

#define SIGN(X) ((X) < 0. ? -1 : ((X) > 0. ? 1 : 0.))

#endif // Qocc_H

