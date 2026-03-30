#ifndef IQCADSKETCHPARAMETER_H
#define IQCADSKETCHPARAMETER_H


#include "iqparameter.h"
#include "cadsketchparameter.h"


class IQCADSketchParameter
    : public IQSpecializedParameter<insight::CADSketchParameter>
{
public:
    declareType(insight::CADSketchParameter::typeName_());

    IQCADSketchParameter
        (
            QObject* parent,
            IQHierarchicalDataModel* hdmodel,
            insight::hierarchicalData::Element* element
            );

    QVariant value() const override;

    QVBoxLayout* populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer) override;

    void populateContextMenu(QMenu* m, IQCADModel3DViewer *viewer) override;

    void edit(IQCADModel3DViewer *viewer);
};


/*
class TOOLKIT_GUI_EXPORT IQCADSketchDelegate
    : public QStyledItemDelegate
{
    Q_OBJECT

public:
    IQCADSketchDelegate(QObject * parent = 0);

    QWidget * createEditor(
        QWidget * parent,
        const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

    void setEditorData(
        QWidget *editor,
        const QModelIndex &index ) const override;

    void setModelData(
        QWidget *editor,
        QAbstractItemModel *model,
        const QModelIndex &index ) const override;

};
*/


#endif // IQCADSKETCHPARAMETER_H
