#ifndef IQCADMODEL3DVIEWER_H
#define IQCADMODEL3DVIEWER_H

#include "insightcad_gui_export.h"


#include "vtkVersionMacros.h"
#if VTK_MAJOR_VERSION>=8
#include <QVTKOpenGLWidget.h>
#else
#include <QVTKWidget.h>
#endif
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkDataObject.h>
#include "vtkImageMapper.h"
#include "vtkImageActor.h"


#include <QAbstractItemModel>

#include "base/boost_include.h"

#include "cadtypes.h"






class PickInteractorStyle
      : public QObject,
        public vtkInteractorStyleTrackballCamera
{
    Q_OBJECT

    PickInteractorStyle();

    int lastClickPos[2];

public:
  static PickInteractorStyle* New();

  vtkTypeMacro(PickInteractorStyle, vtkInteractorStyleTrackballCamera);

  void OnLeftButtonUp() override;
  void OnLeftButtonDown() override;
  void OnRightButtonUp() override;
  void OnRightButtonDown() override;

Q_SIGNALS:
  void contextMenuRequested(vtkActor* actor);

};




typedef
#if VTK_MAJOR_VERSION>=8
QVTKOpenGLWidget
#else
QVTKWidget
#endif
VTKWidget;


class INSIGHTCAD_GUI_EXPORT IQCADModel3DViewer
        : public VTKWidget
{
    Q_OBJECT

public:
    typedef boost::variant<
        insight::cad::DatumPtr,
        insight::cad::FeaturePtr,
        vtkSmartPointer<vtkDataObject>
    > CADEntity;

private:
    vtkSmartPointer<vtkRenderer> ren_;
    QAbstractItemModel* model_;

    struct DisplayedEntity
    {
        QString label_;
        CADEntity ce_;
        vtkSmartPointer<vtkProp> actor_;
    };

    typedef std::map<QPersistentModelIndex, DisplayedEntity> DisplayedData;
    DisplayedData displayedData_;

    void remove(const QPersistentModelIndex& pidx);

    void addDatum(
            const QPersistentModelIndex& pidx,
            const QString& lbl,
            insight::cad::DatumPtr datum );

    void addFeature(
            const QPersistentModelIndex& pidx,
            const QString& lbl,
            insight::cad::FeaturePtr feat );

    void addDataset(
            const QPersistentModelIndex& pidx,
            const QString& lbl,
            vtkSmartPointer<vtkDataObject> ds );

    void resetFeatureDisplayProps(const QPersistentModelIndex& pidx);
    void resetDatasetColor(const QPersistentModelIndex& pidx);

//    QModelIndex modelIndex(const CADEntity& ce) const;

private Q_SLOT:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
    void onModelAboutToBeReset();
    void onRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void onRowsInserted(const QModelIndex &parent, int start, int end);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);

    void addChild(const QModelIndex& idx);
    void addSiblings(const QModelIndex& idx);

Q_SIGNALS:
    void contextMenuRequested(const QModelIndex& index, const QPoint &globalPos);

public:
    IQCADModel3DViewer(QWidget* parent=nullptr);

    void setModel(QAbstractItemModel* model);

    QSize sizeHint() const override;

//    vtkActor* getActor(insight::cad::FeaturePtr geom);
};

#endif // IQCADMODEL3DVIEWER_H
