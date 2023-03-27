#ifndef IQCADMODEL3DVIEWER_H
#define IQCADMODEL3DVIEWER_H

#include "insightcad_gui_export.h"
#include "cadtypes.h"

#include <QAbstractItemModel>
#include <QTextEdit>




class INSIGHTCAD_GUI_EXPORT IQCADModel3DViewer
        : public QWidget
{
    Q_OBJECT
protected:
    QAbstractItemModel* model_;

public:
    IQCADModel3DViewer(QWidget* parent=nullptr);

    virtual void setModel(QAbstractItemModel* model);
    QAbstractItemModel* model() const;

    virtual void connectNotepad(QTextEdit *notepad) const;


    virtual double getScale() const =0;
    virtual void setScale(double s) =0;
    virtual bool pickAtCursor(bool extendSelection) =0;
    virtual void emitGraphicalSelectionChanged() =0;

    virtual QColor getBackgroundColor() const =0;

public Q_SLOT:
    virtual void highlightItem( insight::cad::FeaturePtr feat ) =0;
    virtual void undoHighlightItem() =0;

    virtual void onMeasureDistance() =0;
    virtual void onSelectPoints() =0;
    virtual void onSelectEdges() =0;
    virtual void onSelectFaces() =0;
    virtual void onSelectSolids() =0;

    virtual void toggleClipXY();
    virtual void toggleClipYZ();
    virtual void toggleClipXZ();
    virtual void toggleClipDatum(insight::cad::Datum* pl);
    virtual void toggleClip(const arma::mat& p, const arma::mat& n) =0;

    virtual void fitAll() =0;
    void viewFront();
    void viewBack();
    void viewTop();
    void viewBottom();
    void viewLeft();
    void viewRight();
    virtual void view(const arma::mat& viewDir, const arma::mat& upDir) =0;

    virtual void setBackgroundColor(QColor c) =0;
    virtual void selectBackgroundColor();

    virtual void onlyOneShaded(QPersistentModelIndex idx) =0;
    virtual void resetRepresentations() =0;

Q_SIGNALS:
    void appendToNotepad(const QString& text);
    void contextMenuRequested(const QModelIndex& index, const QPoint &globalPos);

};




#endif // IQCADMODEL3DVIEWER_H