
#include "iqcadmodel3dviewer.h"
#include "constrainedsketch.h"
#include "datum.h"

#include <QApplication>
#include <QColorDialog>
#include <QDockWidget>
#include <QStatusBar>
#include <QResizeEvent>
#include <QLayout>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStyle>
#include <QTabBar>
#include <QTimer>
#include <qnamespace.h>

#include <algorithm>

#include "cadsketchparameter.h"


uint
IQCADModel3DViewer::QPersistentModelIndexHash::operator()
    ( const QPersistentModelIndex& idx ) const
{
    return qHash(idx);
}



IQCADModel3DViewer::IQCADModel3DViewer(QWidget *parent)
    : /*QWidget(parent),*/
      QMainWindow(parent, Qt::Widget), // flag important
      model_(nullptr)
{
    userMessage_ = new QLabel;
    userMessage_->setAlignment(Qt::AlignLeft);
    currentActionDesc_ = new QLabel;
    currentActionDesc_->setAlignment(Qt::AlignRight);

    mouseCoordinateDisplay_= new QLabel;
    mouseCoordinateDisplay_->setAlignment(Qt::AlignCenter);

    statusBar()->addPermanentWidget(userMessage_, 80);
    statusBar()->addPermanentWidget(mouseCoordinateDisplay_, 10);
    statusBar()->addPermanentWidget(currentActionDesc_, 10);
}

void IQCADModel3DViewer::setModel(QAbstractItemModel *model)
{
    model_=model;
}

QAbstractItemModel *IQCADModel3DViewer::model() const
{
    return model_;
}

IQCADItemModel *IQCADModel3DViewer::cadmodel() const
{
    return dynamic_cast<IQCADItemModel*>(model_);
}

void IQCADModel3DViewer::addToolBox(QWidget *w, const QString &title)
{
    if (!toolBoxTabs_)
    {
        toolBoxTabs_ = new QTabWidget;
        toolBoxDock_ = new QDockWidget;
        toolBoxDock_->setWidget(toolBoxTabs_);
        toolBoxDock_->setTitleBarWidget(new QWidget); // tab labels suffice
        addDockWidget(Qt::RightDockWidgetArea, toolBoxDock_);

        connect(toolBoxTabs_, &QTabWidget::currentChanged,
                this, &IQCADModel3DViewer::scheduleToolBoxDockGrowth);

        // remove "new content" mark, once a tab was visited
        connect(toolBoxTabs_, &QTabWidget::currentChanged, toolBoxTabs_,
                [this](int index)
                {
                    if (index>=0)
                        toolBoxTabs_->tabBar()->setTabTextColor(index, QColor());
                });
    }

    // don't take away the tab, the user is currently working in
    // (e.g. multi-selection in model tree)
    auto *cur = toolBoxTabs_->currentWidget();
    auto *fw = QApplication::focusWidget();
    bool userWorksInCurrentTab = cur && fw && cur->isAncestorOf(fw);

    // wrap into scroll area, so that the content size does not
    // enforce the dock width (and thus resize the 3D view)
    auto sa = new QScrollArea;
    sa->setWidgetResizable(true);
    sa->setFrameShape(QFrame::NoFrame);
    sa->setWidget(w);

    // the tab is removed automatically, when the scroll area is deleted.
    // The widget is owned by the caller, so remove its scroll area along with it.
    connect(w, &QObject::destroyed, sa, &QObject::deleteLater);
    // Queued, because the page is removed only after "destroyed" was emitted.
    connect(sa, &QObject::destroyed, toolBoxTabs_,
            [this]()
            {
                if (toolBoxTabs_->count()==0)
                    toolBoxDock_->hide();
            },
            Qt::QueuedConnection );

    // content is usually populated after this function returns:
    // watch for layout changes to adapt dock width
    w->installEventFilter(this);

    // most recent toolbox is leftmost
    int i = toolBoxTabs_->insertTab(0, sa, title);
    if (userWorksInCurrentTab)
    {
        // keep current tab (index shifts automatically), mark the new one
        toolBoxTabs_->tabBar()->setTabTextColor(
            i, palette().color(QPalette::Highlight) );
    }
    else
    {
        toolBoxTabs_->setCurrentIndex(i);
    }
    toolBoxDock_->show();

    scheduleToolBoxDockGrowth();
}


void IQCADModel3DViewer::scheduleToolBoxDockGrowth()
{
    QTimer::singleShot(0, this, &IQCADModel3DViewer::growToolBoxDockIfNeeded);
}


void IQCADModel3DViewer::growToolBoxDockIfNeeded()
{
    // only grow automatically, never shrink.
    // Once the user has set the width, leave it alone.
    if (toolBoxWidthSetByUser_ || !toolBoxDock_ || !toolBoxDock_->isVisible())
        return;

    auto sa = qobject_cast<QScrollArea*>(toolBoxTabs_->currentWidget());
    if (!sa || !sa->widget() || sa->viewport()->width()<=0)
        return;

    auto w = sa->widget();
    int contentWidth = std::max(
        w->sizeHint().width(),
        w->minimumSizeHint().width() );

    // space taken by tab frame, margins etc.
    int overhead = toolBoxDock_->width() - sa->viewport()->width();
    if (!sa->verticalScrollBar()->isVisible())
        overhead += sa->verticalScrollBar()->sizeHint().width();

    int needed = contentWidth + overhead;
    if (needed > toolBoxDock_->width())
    {
        resizeDocks({toolBoxDock_}, {needed}, Qt::Horizontal);
    }
}


bool IQCADModel3DViewer::event(QEvent *e)
{
    // detect dragging of the separator between the view and the toolbox dock
    // by the user. The separator belongs to the main window itself.
    if (e->type()==QEvent::Show)
    {
        scheduleToolBoxDockGrowth();
    }

    if (toolBoxDock_ && toolBoxDock_->isVisible())
    {
        if (e->type()==QEvent::MouseButtonPress)
        {
            auto me = static_cast<QMouseEvent*>(e);
            auto dg = toolBoxDock_->geometry();
            int sepExtent = style()->pixelMetric(
                QStyle::PM_DockWidgetSeparatorExtent, nullptr, this );
            int x = me->pos().x(), y = me->pos().y();
            if ( x >= dg.left()-sepExtent-2 && x < dg.left()
                && y >= dg.top() && y <= dg.bottom() )
            {
                toolBoxWidthAtPress_ = toolBoxDock_->width();
            }
            else
            {
                toolBoxWidthAtPress_ = -1;
            }
        }
        else if (e->type()==QEvent::MouseButtonRelease)
        {
            if ( toolBoxWidthAtPress_>=0
                && toolBoxDock_->width()!=toolBoxWidthAtPress_ )
            {
                toolBoxWidthSetByUser_ = true;
            }
            toolBoxWidthAtPress_ = -1;
        }
    }

    return QMainWindow::event(e);
}


bool IQCADModel3DViewer::eventFilter(QObject *watched, QEvent *e)
{
    if (e->type()==QEvent::LayoutRequest)
    {
        scheduleToolBoxDockGrowth();
    }
    return QMainWindow::eventFilter(watched, e);
}

void IQCADModel3DViewer::connectNotepad(QTextEdit *notepad) const
{
    connect(
       this, &IQCADModel3DViewer::appendToNotepad,
        notepad, &QTextEdit::append );
}


void IQCADModel3DViewer::setSelectionModel(QItemSelectionModel *selmodel)
{
}

// bool IQCADModel3DViewer::onLeftButtonDown(
//     Qt::KeyboardModifiers nFlags,
//     const QPoint point, bool afterDoubleClick ) {return false;}

// bool IQCADModel3DViewer::onKeyPress(
//     Qt::KeyboardModifiers modifiers,
//     int key ) {return false;}

// bool IQCADModel3DViewer::onLeftButtonUp(
//     Qt::KeyboardModifiers nFlags,
//     const QPoint point,
//     bool lastClickWasDoubleClick ) {return false;}

void IQCADModel3DViewer::toggleClipXY()
{
    toggleClip( insight::vec3Zero(), insight::vec3Z() );
}

void IQCADModel3DViewer::toggleClipXZ()
{
    toggleClip( insight::vec3Zero(), insight::vec3Y() );
}

void IQCADModel3DViewer::toggleClipYZ()
{
    toggleClip( insight::vec3Zero(), insight::vec3X() );
}


void IQCADModel3DViewer::toggleClipDatum(insight::cad::Datum* dat)
{
    if (dat->providesPlanarReference())
    {
        auto pl = dat->plane();
        toggleClip(
                    insight::vec3(pl.Location()),
                    insight::vec3(pl.Direction()) );
    }
}


void IQCADModel3DViewer::viewFront()
{
    view( insight::vec3X(1), insight::vec3Z() );
}

void IQCADModel3DViewer::viewBack()
{
    view( insight::vec3X(-1), insight::vec3Z() );
}

void IQCADModel3DViewer::viewTop()
{
    view( insight::vec3Z(-1), insight::vec3Y() );
}

void IQCADModel3DViewer::viewBottom()
{
    view( insight::vec3Z(1), insight::vec3Y() );
}

void IQCADModel3DViewer::viewLeft()
{
    view( insight::vec3Y(-1), insight::vec3Z() );
}

void IQCADModel3DViewer::viewRight()
{
    view( insight::vec3Y(1), insight::vec3Z() );
}

void IQCADModel3DViewer::selectBackgroundColor()
{
    QColor aColor = getBackgroundColor();

    QColor aRetColor = QColorDialog::getColor(aColor);

    if( aRetColor.isValid() )
    {
        setBackgroundColor(aRetColor);
    }
}

std::shared_ptr<IQParameterSetModel::EditingDisabler>
IQCADModel3DViewer::editSketchParameter(
    const std::string& parameterPath,
    std::shared_ptr<insight::cad::ConstrainedSketch> sketchOvr )
{
    std::shared_ptr<IQParameterSetModel::EditingDisabler> editctrl;

    if ( auto psm = dynamic_cast<IQParameterSetModel*>(
            cadmodel()->associatedParameterSetModel()) )
    {
        auto &skp = dynamic_cast<insight::CADSketchParameter&>(
            psm->parameterRef(parameterPath) );

        editctrl = psm->disableEditing();

        editSketch(

            sketchOvr ? *sketchOvr : skp.sketch(),

            skp.entityProperties(),
            skp.presentationDelegateKey(),

            [this,parameterPath,psm,
             editctrl /*capture handle, shall be deleted, when this function is deleted, i.e. sketch editor finshed*/]
            (insight::cad::ConstrainedSketchPtr accSk) // on accept
            {
                auto &skp = dynamic_cast<insight::CADSketchParameter&>(
                    psm->parameterRef(parameterPath) );

                {
                    auto bulkGuard = psm->beginBulkUpdate();

                    std::ostringstream os;
                    accSk->generateScript(os);

                    skp.setScript(os.str());
                    skp.sketch(); //trigger rebuild

                    // bulkGuard destroyed at end of scope →
                    // bulkUpdateFinished → single rebuildVisualization + autosave
                }
            },

            [](insight::cad::ConstrainedSketchPtr) // on cancel
            {},

            parameterPath
        );
    }

    return editctrl;
}

void IQCADModel3DViewer::showCurrentActionDescription(const QString& desc)
{
    currentActionDesc_->setText(desc);
    userMessage_->setText(QString());
}

void IQCADModel3DViewer::showUserPrompt(const QString &text)
{
    userMessage_->setText(text);
}

void IQCADModel3DViewer::updateMouseCoordinateDisplay(double x, double y)
{
    mouseCoordinateDisplay_->setText(
        QString("X=%1, Y=%2")
            .arg(x,6)
            .arg(y,6)
        );
}
