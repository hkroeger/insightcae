#include "iqzoomableimageview.h"
#include "base/translations.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QGuiApplication>
#include <QScreen>



IQZoomPanGraphicsView::IQZoomPanGraphicsView(QWidget *parent)
    : QGraphicsView(parent),
      scene_(new QGraphicsScene(this)),
      pixmapItem_(nullptr),
      fitToWindow_(true),
      minScale_(0.05),
      maxScale_(20.0),
      zoomStepFactor_(1.15)
{
    setScene(scene_);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setFrameShape(QFrame::NoFrame);
    setAlignment(Qt::AlignCenter);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void IQZoomPanGraphicsView::setPixmap(const QPixmap &pm)
{
    scene_->clear();
    pixmapItem_ = nullptr;

    if (!pm.isNull())
    {
        pixmapItem_ = scene_->addPixmap(pm);
        scene_->setSceneRect(pixmapItem_->boundingRect());
    }

    fitToWindow_ = true;
    doFitInView();
}

bool IQZoomPanGraphicsView::hasPixmap() const
{
    return pixmapItem_ && !pixmapItem_->pixmap().isNull();
}

bool IQZoomPanGraphicsView::isFitToWindow() const
{
    return fitToWindow_;
}

void IQZoomPanGraphicsView::doFitInView()
{
    if (hasPixmap())
        fitInView(pixmapItem_, Qt::KeepAspectRatio);
}

void IQZoomPanGraphicsView::applyRelativeZoom(double factor)
{
    double curScale = transform().m11();
    double newScale = qBound(minScale_, curScale*factor, maxScale_);
    if (qFuzzyCompare(newScale, curScale))
        return;
    double appliedFactor = newScale/curScale;
    scale(appliedFactor, appliedFactor);
}

void IQZoomPanGraphicsView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier)
    {
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        double factor = (event->angleDelta().y()>0) ?
                    zoomStepFactor_ : 1.0/zoomStepFactor_;
        applyRelativeZoom(factor);
        fitToWindow_ = false;
        event->accept();
    }
    else
    {
        // do not forward to QGraphicsView::wheelEvent: it would consume the
        // event even while the whole scene already fits into the viewport,
        // preventing the enclosing (vertically scrolling) result panel from
        // receiving normal page-scroll wheel events
        event->ignore();
    }
}

void IQZoomPanGraphicsView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (fitToWindow_)
        doFitInView();
}

void IQZoomPanGraphicsView::zoomIn()
{
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    applyRelativeZoom(zoomStepFactor_);
    fitToWindow_ = false;
}

void IQZoomPanGraphicsView::zoomOut()
{
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    applyRelativeZoom(1.0/zoomStepFactor_);
    fitToWindow_ = false;
}

void IQZoomPanGraphicsView::resetZoom()
{
    fitToWindow_ = true;
    doFitInView();
}




IQZoomableImageView::IQZoomableImageView(QWidget *parent)
    : QWidget(parent),
      view_(new IQZoomPanGraphicsView(this))
{
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0,0,0,0);
    outerLayout->setSpacing(2);

    auto *toolbarLayout = new QHBoxLayout;
    toolbarLayout->setContentsMargins(0,0,0,0);

    zoomInButton_ = new QToolButton(this);
    zoomInButton_->setIcon(QIcon(":/icons/icon_zoom_in.svg"));
    zoomInButton_->setToolTip(_("Zoom in"));
    connect(zoomInButton_, &QToolButton::clicked, view_, &IQZoomPanGraphicsView::zoomIn);

    zoomOutButton_ = new QToolButton(this);
    zoomOutButton_->setIcon(QIcon(":/icons/icon_zoom_out.svg"));
    zoomOutButton_->setToolTip(_("Zoom out"));
    connect(zoomOutButton_, &QToolButton::clicked, view_, &IQZoomPanGraphicsView::zoomOut);

    resetZoomButton_ = new QToolButton(this);
    resetZoomButton_->setIcon(QIcon(":/icons/icon_zoom_reset.svg"));
    resetZoomButton_->setToolTip(_("Reset zoom (fit whole image into view)"));
    connect(resetZoomButton_, &QToolButton::clicked, view_, &IQZoomPanGraphicsView::resetZoom);

    toolbarLayout->addWidget(zoomInButton_);
    toolbarLayout->addWidget(zoomOutButton_);
    toolbarLayout->addWidget(resetZoomButton_);
    toolbarLayout->addStretch(1);

    outerLayout->addLayout(toolbarLayout);
    outerLayout->addWidget(view_);

    setMinimumSize(1,1);
    QSizePolicy qsp(QSizePolicy::Preferred, QSizePolicy::Preferred);
    qsp.setHeightForWidth(true);
    setSizePolicy(qsp);
}

IQZoomableImageView::IQZoomableImageView(const QPixmap &pm, QWidget *parent)
    : IQZoomableImageView(parent)
{
    setPixmap(pm);
}

void IQZoomableImageView::setPixmap(const QPixmap &pm)
{
    pixmap_ = pm;
    view_->setPixmap(pm);
    updateGeometry();
}

const QPixmap &IQZoomableImageView::originalPixmap() const
{
    return pixmap_;
}

int IQZoomableImageView::maxImageAreaHeight() const
{
    int avail = 800;
    if (auto *screen = QGuiApplication::primaryScreen())
        avail = screen->availableGeometry().height();
    return qMax(200, int(avail*0.6));
}

int IQZoomableImageView::heightForWidth(int width) const
{
    int toolbarH = zoomInButton_->sizeHint().height();
    int spacing = layout() ? layout()->spacing() : 0;

    if (pixmap_.isNull())
        return toolbarH;

    int aspectH = double(pixmap_.height())*width/double(pixmap_.width());
    int imgH = qBound(1, aspectH, maxImageAreaHeight());
    return toolbarH + spacing + imgH;
}

QSize IQZoomableImageView::sizeHint() const
{
    return QSize(width(), heightForWidth(width()));
}
