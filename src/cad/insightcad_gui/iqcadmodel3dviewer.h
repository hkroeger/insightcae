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

#include <QAbstractItemModel>

#include "base/boost_include.h"

#include "cadtypes.h"






class PickInteractorStyle : public vtkInteractorStyleTrackballCamera
{

    PickInteractorStyle();

    int lastClickPos[2];

public:
  static PickInteractorStyle* New();

  vtkTypeMacro(PickInteractorStyle, vtkInteractorStyleTrackballCamera);

  void OnLeftButtonUp() override;
  void OnLeftButtonDown() override;

//  void OnKeyDown() override
//  {
//      this->GetInteractor()->
//  }

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
public:
    typedef boost::variant<insight::cad::DatumPtr,insight::cad::FeaturePtr> CADEntity;

private:
    int visibilityCol_, labelCol_, entityCol_;

    vtkSmartPointer<vtkRenderer> ren_;
    QAbstractItemModel* model_;

    struct DisplayedEntity
    {
        QString label_;
        vtkSmartPointer<vtkMapper> mapper_;
        vtkSmartPointer<vtkActor> actor_;
    };

    std::map<CADEntity, DisplayedEntity> displayedData_;

    void remove(std::map<CADEntity, DisplayedEntity>::iterator i);

    DisplayedEntity& addDatum(const QString& lbl, insight::cad::DatumPtr datum);
    DisplayedEntity& addFeature(const QString& lbl, insight::cad::FeaturePtr feat);

private Q_SLOT:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
    void onModelAboutToBeReset();
    void onRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void onRowsInserted(const QModelIndex &parent, int start, int end);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);

    void addChild(const QModelIndex& idx);
    void addSiblings(const QModelIndex& idx);

Q_SIGNALS:
    void contextMenuRequested(const QPoint &p, QModelIndex index);

public:
    IQCADModel3DViewer(QWidget* parent=nullptr, int visibilityCol=0, int labelCol=1, int entityCol=3);

    void setModel(QAbstractItemModel* model);

    QSize sizeHint() const override;

};

#endif // IQCADMODEL3DVIEWER_H
