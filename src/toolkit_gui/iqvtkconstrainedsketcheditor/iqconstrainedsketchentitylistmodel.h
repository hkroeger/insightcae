#ifndef IQCONSTRAINEDSKETCHENTITYLISTMODEL_H
#define IQCONSTRAINEDSKETCHENTITYLISTMODEL_H

#include "toolkit_gui_export.h"

#include <QAbstractTableModel>

#include "iqvtkconstrainedsketcheditor.h"

class IQConstrainedSketchEntityListModel
    : public QAbstractTableModel
{
    Q_OBJECT

    IQVTKConstrainedSketchEditor *sketchEditor_;
    std::map<int, insight::cad::ConstrainedSketchEntity*> geometry_;

public:
    IQConstrainedSketchEntityListModel(
        IQVTKConstrainedSketchEditor *sk,
        QObject *parent = nullptr );

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;


public Q_SLOTS:
    void update();
};

#endif // IQCONSTRAINEDSKETCHENTITYLISTMODEL_H
