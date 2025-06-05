#ifndef IQCADMODEL3DVIEWER_H
#define IQCADMODEL3DVIEWER_H

#include "toolkit_gui_export.h"
#include "cadmodel3dviewer.h"

#include <QAbstractItemModel>
#include <QTextEdit>
#include <QMainWindow>
#include <QToolBox>
#include <QLabel>

#include "iqcaditemmodel.h"
#include "constrainedsketch.h"




class QItemSelectionModel;

class IQVTKViewerState;

class TOOLKIT_GUI_EXPORT IQCADModel3DViewer
    : public QMainWindow, //for toolbars to work //QWidget
      public CADModel3DViewer
{
public:
    typedef std::function<void(insight::cad::ConstrainedSketchPtr)>
        SketchCompletionCallback;


    struct QPersistentModelIndexHash
    {
        uint operator()(const QPersistentModelIndex& idx) const;
    };


private:
    Q_OBJECT

protected:
    QAbstractItemModel* model_;
    QLabel *userMessage_, *currentActionDesc_, *mouseCoordinateDisplay_;

public:
    IQCADModel3DViewer(QWidget* parent=nullptr);

    virtual void setModel(QAbstractItemModel* model);
    QAbstractItemModel* model() const;
    IQCADItemModel* cadmodel() const;

    // inline QToolBox *commonToolBox() { return commonToolBox_; }
    void addToolBox(QWidget* w, const QString& title);

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

    // virtual bool onLeftButtonDown(
    //     Qt::KeyboardModifiers nFlags,
    //     const QPoint point, bool afterDoubleClick );
    // virtual bool onKeyPress(
    //     Qt::KeyboardModifiers modifiers,
    //     int key );
    // virtual bool onLeftButtonUp(
    //     Qt::KeyboardModifiers nFlags,
    //     const QPoint point,
    //     bool lastClickWasDoubleClick );

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
            const insight::cad::ConstrainedSketch& sketch,
            std::shared_ptr<insight::cad::ConstrainedSketchParametersDelegate> entityProperties,
            const std::string& presentationDelegateKey,
            SketchCompletionCallback onAccept,
            SketchCompletionCallback onCancel = [](insight::cad::ConstrainedSketchPtr) {},
            boost::optional<std::string> parameterPath = boost::optional<std::string>()
        ) =0;

    virtual void editSketchParameter(
        const std::string& parameterPath,
        std::shared_ptr<insight::cad::ConstrainedSketch> sketchOvr = nullptr );

    void showCurrentActionDescription(const QString& desc);
    void showUserPrompt(const QString& text);
    void updateMouseCoordinateDisplay(double x, double y);

Q_SIGNALS:
    void appendToNotepad(const QString& text);
    void contextMenuRequested(const QModelIndex& index, const QPoint &globalPos);

};




#endif // IQCADMODEL3DVIEWER_H
