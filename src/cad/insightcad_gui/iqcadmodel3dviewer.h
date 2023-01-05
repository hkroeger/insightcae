#ifndef IQCADMODEL3DVIEWER_H
#define IQCADMODEL3DVIEWER_H

#include "insightcad_gui_export.h"


#include "vtkVersionMacros.h"
#include "vtkGenericOpenGLRenderWindow.h"
#if VTK_MAJOR_VERSION>=8
#include <QVTKOpenGLWidget.h>
#else
#include <QVTKWidget.h>
#endif
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkDataObject.h>


#include <QAbstractItemModel>


#include "cadtypes.h"

#include <unordered_map>


class vtkRenderWindowInteractor;

typedef
#if VTK_MAJOR_VERSION>=8
QVTKOpenGLWidget
#else
QVTKWidget
#endif
VTKWidget;


class INSIGHTCAD_GUI_EXPORT IQCADModel3DViewer
        : public QWidget
{
    Q_OBJECT

public:
    typedef boost::variant<
        insight::cad::VectorPtr,
        insight::cad::DatumPtr,
        insight::cad::FeaturePtr,
        vtkSmartPointer<vtkDataObject>
    > CADEntity;

private:
    VTKWidget vtkWidget_;
    vtkSmartPointer<vtkRenderer> ren_;
    QAbstractItemModel* model_;

    struct DisplayedEntity
    {
        QString label_;
        CADEntity ce_;
        vtkSmartPointer<vtkProp> actor_;
    };

    struct QPersistentModelIndexHash
    {
        uint operator()(const QPersistentModelIndex& idx) const
        {
            return qHash(idx);
        }
    };

    typedef std::unordered_map<QPersistentModelIndex, DisplayedEntity, QPersistentModelIndexHash > DisplayedData;
    DisplayedData displayedData_;



    class HighlightItem
    {
        IQCADModel3DViewer* viewer_;

        CADEntity entity_;
        std::shared_ptr<DisplayedEntity> de_;
        QPersistentModelIndex idx2highlight_;

    public:
        HighlightItem(
                std::shared_ptr<DisplayedEntity> de,
                QPersistentModelIndex idx2highlight,
                IQCADModel3DViewer* viewer);
        ~HighlightItem();

        const CADEntity& entity() const;
        QModelIndex index() const;

    };
    friend class HighlightItem;

    mutable std::shared_ptr< HighlightItem > highlightedItem_;



    void remove(const QPersistentModelIndex& pidx);

    vtkSmartPointer<vtkProp> createActor(CADEntity entity) const;

    void addVertex(
            const QPersistentModelIndex& pidx,
            const QString& lbl,
            insight::cad::VectorPtr point );

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

    void resetVisibility(const QPersistentModelIndex& pidx);
    void resetFeatureDisplayProps(const QPersistentModelIndex& pidx);
    void resetDatasetColor(const QPersistentModelIndex& pidx);

    void doHighlightItem( CADEntity item );

private Q_SLOT:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
    void onModelAboutToBeReset();
    void onRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void onRowsInserted(const QModelIndex &parent, int start, int end);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);

    void addChild(const QModelIndex& idx);
    void addSiblings(const QModelIndex& idx);

public Q_SLOT:
    void highlightItem( insight::cad::FeaturePtr feat );
    void undoHighlightItem();

Q_SIGNALS:
    void contextMenuRequested(const QModelIndex& index, const QPoint &globalPos);

public:
    IQCADModel3DViewer(QWidget* parent=nullptr);

    void setModel(QAbstractItemModel* model);

    vtkRenderWindowInteractor* interactor() const;

    QSize sizeHint() const override;

//    vtkActor* getActor(insight::cad::FeaturePtr geom);
};

#endif // IQCADMODEL3DVIEWER_H
