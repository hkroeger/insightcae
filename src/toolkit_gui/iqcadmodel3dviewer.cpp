
#include "iqcadmodel3dviewer.h"
#include "datum.h"

#include <QColorDialog>


IQCADModel3DViewer::IQCADModel3DViewer(QWidget *parent)
    : /*QWidget(parent),*/
      QMainWindow(parent, Qt::Widget), // flag important
      model_(nullptr)
{}

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

void IQCADModel3DViewer::connectNotepad(QTextEdit *notepad) const
{
    connect(
       this, &IQCADModel3DViewer::appendToNotepad,
        notepad, &QTextEdit::append );
}


void IQCADModel3DViewer::setSelectionModel(QItemSelectionModel *selmodel)
{
}



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

