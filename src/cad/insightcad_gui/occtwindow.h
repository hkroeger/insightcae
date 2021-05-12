#ifndef OcctWindow_H
#define OcctWindow_H

#include <Standard_Version.hxx>
#include <Aspect_Window.hxx>

#if OCC_VERSION_MAJOR>=7
#include <Standard_WarningsDisable.hxx>
#endif
#include <QWidget>
#if OCC_VERSION_MAJOR>=7
#include <Standard_WarningsRestore.hxx>
#endif

class OcctWindow;

/*
  OcctWindow class implements Aspect_Window interface using Qt API 
  as a platform-independent source of window geometry information. 
  A similar class should be used instead of platform-specific OCCT 
  classes (WNT_Window, Xw_Window) in any Qt 5 application using OCCT 
  3D visualization.

  With Qt 5, the requirement for a Qt-based application to rely fully 
  on Qt public API and stop using platform-specific APIs looks mandatory. 
  An example of this is changed QWidget event sequence: when a widget is 
  first shown on the screen, a resize event is generated before the 
  underlying native window is resized correctly, however the QWidget instance
  already holds correct size information at that moment. The OCCT classes 
  acting as a source of window geometry for V3d_View class (WNT_Window, Xw_Window) 
  are no longer compatible with changed Qt behavior because they rely on 
  platform-specific API that cannot return correct window geometry information 
  in some cases. A reasonable solution is to provide a Qt-based implementation 
  of Aspect_Window interface at application level.
*/

class OcctWindow : public Aspect_Window
{

protected:
  mutable Standard_Integer xLeft_;
  mutable Standard_Integer yTop_;
  mutable Standard_Integer xRight_;
  mutable Standard_Integer yBottom_;
  QWidget* widget_;


  QRect widgetRect() const;

public:

  //! Constructor
  OcctWindow( QWidget* widget, const Quantity_NameOfColor backColor = Quantity_NOC_MATRAGRAY );

  void Destroy()
#if OCC_VERSION_MAJOR<7
  override
#endif
  ;

  //! Destructor
  ~OcctWindow();

  //! Returns native Window handle
  Aspect_Drawable NativeHandle() const override;

  //! Returns parent of native Window handle.
  Aspect_Drawable NativeParentHandle() const override;

  //! Applies the resizing to the window <me>
  Aspect_TypeOfResize DoResize()
#if OCC_VERSION_MAJOR<7
  const
#endif
  override;

  //! Returns True if the window <me> is opened
  //! and False if the window is closed.
  Standard_Boolean IsMapped() const override;

  //! Apply the mapping change to the window <me>
  //! and returns TRUE if the window is mapped at screen.
  Standard_Boolean DoMapping() const override;

  //! Opens the window <me>.
  void Map() const override;

  //! Closes the window <me>.
  void Unmap() const override;

  void Position( Standard_Integer& theX1, Standard_Integer& theY1,
                 Standard_Integer& theX2, Standard_Integer& theY2 ) const override;

  //! Returns The Window RATIO equal to the physical
  //! WIDTH/HEIGHT dimensions.
  Standard_Real Ratio() const override;

  void Size( Standard_Integer& theWidth, Standard_Integer& theHeight ) const override;

#if OCC_VERSION_MAJOR>=7
  Aspect_FBConfig NativeFBConfig() const override;
#endif


};


#endif // OcctWindow_H
