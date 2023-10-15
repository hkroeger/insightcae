#ifndef IQCADMODEL3DVIEWER_H
#define IQCADMODEL3DVIEWER_H

#include "toolkit_gui_export.h"
#include "cadtypes.h"

#include <QAbstractItemModel>
#include <QTextEdit>
#include <QMainWindow>

#include "iqcaditemmodel.h"
#include "constrainedsketch.h"

class QItemSelectionModel;

class IQVTKViewerState;


class TOOLKIT_GUI_EXPORT IQCADModel3DViewer
        : public QMainWindow //for toolbars to work //QWidget
{
public:
    typedef std::function<void(const insight::ParameterSet& seps, vtkProperty* actprops)>
        SetSketchEntityAppearanceCallback;
    typedef std::function<void()>
        SketchCompletionCallback;


    struct QPersistentModelIndexHash
    {
        uint operator()(const QPersistentModelIndex& idx) const;
    };

    typedef boost::variant<
        insight::cad::VectorPtr,
        insight::cad::DatumPtr,
        insight::cad::FeaturePtr,
        insight::cad::PostprocActionPtr,
        vtkSmartPointer<vtkDataObject>
        > CADEntity;

    typedef boost::variant<
        std::weak_ptr<insight::cad::Vector>,
        std::weak_ptr<insight::cad::Datum>,
        std::weak_ptr<insight::cad::Feature>,
        std::weak_ptr<insight::cad::PostprocAction>,
        vtkWeakPointer<vtkDataObject>
        > CADEntityWeakPtr;


private:
    Q_OBJECT

protected:
    QAbstractItemModel* model_;

public:
    IQCADModel3DViewer(QWidget* parent=nullptr);

    virtual void setModel(QAbstractItemModel* model);
    QAbstractItemModel* model() const;
    IQCADItemModel* cadmodel() const;

    virtual void connectNotepad(QTextEdit *notepad) const;


    virtual double getScale() const =0;
    virtual void setScale(double s) =0;
    virtual bool pickAtCursor(bool extendSelection) =0;
    virtual void emitGraphicalSelectionChanged() =0;

    virtual QColor getBackgroundColor() const =0;

    virtual void setSelectionModel(QItemSelectionModel *selmodel);

public Q_SLOT:
    virtual void exposeItem( insight::cad::FeaturePtr feat ) =0;
    virtual void undoExposeItem() =0;

    virtual bool onLeftButtonDown(
        Qt::KeyboardModifiers nFlags,
        const QPoint point );
    virtual bool onKeyPress(
        Qt::KeyboardModifiers modifiers,
        int key );
    virtual bool onLeftButtonUp(
        Qt::KeyboardModifiers nFlags,
        const QPoint point );

    virtual void onMeasureDistance() =0;
    virtual void onMeasureDiameter() =0;
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

    virtual void doSketchOnPlane(insight::cad::DatumPtr plane) =0;
    virtual void editSketch(
            insight::cad::ConstrainedSketchPtr sketch,
            const insight::ParameterSet& defaultGeometryParameters,
            SetSketchEntityAppearanceCallback saac,
            SketchCompletionCallback scc ) =0;

Q_SIGNALS:
    void appendToNotepad(const QString& text);
    void contextMenuRequested(const QModelIndex& index, const QPoint &globalPos);

};




#endif // IQCADMODEL3DVIEWER_H
