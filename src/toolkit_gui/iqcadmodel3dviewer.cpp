
#include "iqcadmodel3dviewer.h"
#include "datum.h"

#include <QColorDialog>
#include <QDockWidget>
#include <QStatusBar>
#include <QResizeEvent>
#include <QLayout>
#include <qnamespace.h>

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

    statusBar()->addPermanentWidget(userMessage_, 90);
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
    auto dw=new QDockWidget(title);
    dw->setWidget(w);
    connect(w, &QObject::destroyed, dw, &QWidget::deleteLater);
    addDockWidget(Qt::RightDockWidgetArea, dw);
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

void IQCADModel3DViewer::showCurrentActionDescription(const QString& desc)
{
    currentActionDesc_->setText(desc);
    userMessage_->setText(QString());
}

void IQCADModel3DViewer::showUserPrompt(const QString &text)
{
    userMessage_->setText(text);
}

