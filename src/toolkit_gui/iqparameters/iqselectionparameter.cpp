#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>

#include "iqselectionparameter.h"
#include "iqparametersetmodel.h"




defineTemplateType(IQSelectionParameter);
addToFactoryTable(IQParameter, IQSelectionParameter);




addFunctionToStaticFunctionTable(
    IQParameterGridViewDelegateEditorWidget, IQSelectionParameter,
    createDelegate,
    [](QObject* parent) { return new IQSelectionDelegate(parent); }
    );




IQSelectionDelegate::IQSelectionDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}




QWidget *IQSelectionDelegate::createEditor(
    QWidget *parent,
    const QStyleOptionViewItem &option,
    const QModelIndex &index ) const
{
    auto*cb= new QComboBox(parent);
    connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &IQSelectionDelegate::commitAndClose );
    return cb;
}




void IQSelectionDelegate::setEditorData(
    QWidget *editor,
    const QModelIndex &index) const
{
    auto *cb=dynamic_cast<QComboBox*>(editor);
    auto *iqp=static_cast<IQParameter*>(
        index.siblingAtColumn(IQParameterSetModel::iqParamCol)
            .data().value<void*>() );
    auto &iqssp=dynamic_cast<IQSelectionParameterInterface&>(*iqp);

    cb->addItems(iqssp.selectionKeys());
    cb->setCurrentIndex(iqssp.selectionParameter().selectionIndex());
}




void IQSelectionDelegate::setModelData(
    QWidget *editor,
    QAbstractItemModel *model,
    const QModelIndex &index) const
{
    auto *cb=dynamic_cast<QComboBox*>(editor);
    model->setData(index, cb->currentIndex(), Qt::EditRole);
}




void IQSelectionDelegate::commitAndClose(int)
{
    auto *editor = static_cast<QWidget*>(sender());
    Q_EMIT commitData(editor);
    Q_EMIT closeEditor(editor);
}





