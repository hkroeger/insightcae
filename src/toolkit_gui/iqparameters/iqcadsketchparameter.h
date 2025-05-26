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
            IQParameterSetModel* psmodel,
            insight::Parameter* parameter,
            const insight::ParameterSet& defaultParameterSet
            );

    void connectSignals() override;

    QString valueText() const override;

    QVBoxLayout* populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer) override;
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
