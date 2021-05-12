#include <occtwindow.h>


// =======================================================================
// function : OcctWindow
// purpose  :
// =======================================================================
OcctWindow::OcctWindow ( QWidget* widget, const Quantity_NameOfColor backColor )
: Aspect_Window(),
  widget_( widget )
{
  SetBackground (backColor);

  auto rect = widgetRect();
  xLeft_   = rect.left();
  yTop_    = rect.top();
  xRight_  = rect.right();
  yBottom_ = rect.bottom();
}

// =======================================================================
// function : Destroy
// purpose  :
// =======================================================================
void OcctWindow::Destroy()
{
  widget_ = NULL;
}

QRect OcctWindow::widgetRect() const
{
  return widget_->rect();
}

OcctWindow::~OcctWindow()
{
  Destroy();
}

// =======================================================================
// function : NativeParentHandle
// purpose  :
// =======================================================================
Aspect_Drawable OcctWindow::NativeParentHandle() const
{
  QWidget* parentWidget = widget_->parentWidget();
  if ( parentWidget != NULL )
    return (Aspect_Drawable)parentWidget->winId();
  else
    return 0;
}

// =======================================================================
// function : NativeHandle
// purpose  :
// =======================================================================
Aspect_Drawable OcctWindow::NativeHandle() const
{
  return (Aspect_Drawable)widget_->winId();
}

// =======================================================================
// function : IsMapped
// purpose  :
// =======================================================================
Standard_Boolean OcctWindow::IsMapped() const
{
  return !( widget_->isMinimized() || widget_->isHidden() );
}

Standard_Boolean OcctWindow::DoMapping() const
{
  return Standard_True;
}

// =======================================================================
// function : Map
// purpose  :
// =======================================================================
void OcctWindow::Map() const
{
  widget_->show();
  widget_->update();
}

// =======================================================================
// function : Unmap
// purpose  :
// =======================================================================
void OcctWindow::Unmap() const
{
  widget_->hide();
  widget_->update();
}

// =======================================================================
// function : DoResize
// purpose  :
// =======================================================================
Aspect_TypeOfResize OcctWindow::DoResize()
#if OCC_VERSION_MAJOR<7
const
#endif
{
  int                 aMask = 0;
  Aspect_TypeOfResize aMode = Aspect_TOR_UNKNOWN;

  if ( !widget_->isMinimized() )
  {
    auto rect = widgetRect();

    if ( Abs ( rect.left()   - xLeft_   ) > 2 ) aMask |= 1;
    if ( Abs ( rect.right()  - xRight_  ) > 2 ) aMask |= 2;
    if ( Abs ( rect.top()    - yTop_    ) > 2 ) aMask |= 4;
    if ( Abs ( rect.bottom() - yBottom_ ) > 2 ) aMask |= 8;

    switch ( aMask )
    {
      case 0:
        aMode = Aspect_TOR_NO_BORDER;
        break;
      case 1:
        aMode = Aspect_TOR_LEFT_BORDER;
        break;
      case 2:
        aMode = Aspect_TOR_RIGHT_BORDER;
        break;
      case 4:
        aMode = Aspect_TOR_TOP_BORDER;
        break;
      case 5:
        aMode = Aspect_TOR_LEFT_AND_TOP_BORDER;
        break;
      case 6:
        aMode = Aspect_TOR_TOP_AND_RIGHT_BORDER;
        break;
      case 8:
        aMode = Aspect_TOR_BOTTOM_BORDER;
        break;
      case 9:
        aMode = Aspect_TOR_BOTTOM_AND_LEFT_BORDER;
        break;
      case 10:
        aMode = Aspect_TOR_RIGHT_AND_BOTTOM_BORDER;
        break;
      default:
        break;
    }  // end switch

    xLeft_   = rect.left();
    xRight_  = rect.right();
    yTop_    = rect.top();
    yBottom_ = rect.bottom();
  }

  return aMode;
}

// =======================================================================
// function : Ratio
// purpose  :
// =======================================================================
Standard_Real OcctWindow::Ratio() const
{
  QRect rect = widgetRect();
  return Standard_Real( rect.right() - rect.left() ) / Standard_Real( rect.bottom() - rect.top() );
}

// =======================================================================
// function : Size
// purpose  :
// =======================================================================
void OcctWindow::Size ( Standard_Integer& theWidth, Standard_Integer& theHeight ) const
{
  QRect rect = widgetRect();
  theWidth  = rect.width();
  theHeight = rect.height();
}


#if OCC_VERSION_MAJOR>=7
Aspect_FBConfig OcctWindow::NativeFBConfig() const
{
  return nullptr;
}
#endif

// =======================================================================
// function : Position
// purpose  :
// =======================================================================
void OcctWindow::Position ( Standard_Integer& theX1, Standard_Integer& theY1,
                            Standard_Integer& theX2, Standard_Integer& theY2 ) const
{
  auto rect = widgetRect();
  theX1 = rect.left();
  theX2 = rect.right();
  theY1 = rect.top();
  theY2 = rect.bottom();
}
