#ifndef IQZOOMABLEIMAGEVIEW_H
#define IQZOOMABLEIMAGEVIEW_H

#include "toolkit_gui_export.h"

#include <QWidget>
#include <QGraphicsView>
#include <QPixmap>

class QGraphicsScene;
class QGraphicsPixmapItem;
class QToolButton;
class QWheelEvent;
class QResizeEvent;


/**
 * @brief The IQZoomPanGraphicsView class
 * Displays a single pixmap.
 * Defaults to "fit to view" (whole image visible, aspect ratio kept,
 * letterboxed as needed). Ctrl+MouseWheel zooms in/out, anchored under
 * the mouse cursor. Left mouse button drag pans the image.
 * Plain (non-Ctrl) wheel events are ignored so they bubble up to an
 * enclosing scroll area instead of being swallowed by this view.
 */
class TOOLKIT_GUI_EXPORT IQZoomPanGraphicsView
    : public QGraphicsView
{
    Q_OBJECT

    QGraphicsScene *scene_;
    QGraphicsPixmapItem *pixmapItem_;

    bool fitToWindow_;
    const double minScale_;
    const double maxScale_;
    const double zoomStepFactor_;

    void applyRelativeZoom(double factor);
    void doFitInView();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

public:
    explicit IQZoomPanGraphicsView(QWidget *parent = nullptr);

    void setPixmap(const QPixmap &pm);
    bool hasPixmap() const;
    bool isFitToWindow() const;

public Q_SLOTS:
    void zoomIn();
    void zoomOut();
    void resetZoom();
};


/**
 * @brief The IQZoomableImageView class
 * Composite widget used to display insight::Image result elements:
 * a small toolbar (zoom in / zoom out / reset) above an
 * IQZoomPanGraphicsView. Participates in heightForWidth layouts the
 * same way a plain QLabel would, but fits the whole image into a
 * bounded, letterboxed area instead of always scaling to the container
 * width.
 */
class TOOLKIT_GUI_EXPORT IQZoomableImageView
    : public QWidget
{
    Q_OBJECT

    QPixmap pixmap_;
    IQZoomPanGraphicsView *view_;
    QToolButton *zoomInButton_;
    QToolButton *zoomOutButton_;
    QToolButton *resetZoomButton_;

    int maxImageAreaHeight() const;

public:
    explicit IQZoomableImageView(QWidget *parent = nullptr);
    explicit IQZoomableImageView(const QPixmap &pm, QWidget *parent = nullptr);

    void setPixmap(const QPixmap &pm);
    const QPixmap& originalPixmap() const;

    int heightForWidth(int width) const override;
    QSize sizeHint() const override;
};

#endif // IQZOOMABLEIMAGEVIEW_H
