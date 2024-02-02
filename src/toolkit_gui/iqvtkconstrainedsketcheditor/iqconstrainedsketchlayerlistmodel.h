#ifndef IQCONSTRAINEDSKETCHLAYERLISTMODEL_H
#define IQCONSTRAINEDSKETCHLAYERLISTMODEL_H

#include "toolkit_gui_export.h"

#include <QAbstractTableModel>

#include "iqvtkconstrainedsketcheditor.h"

class TOOLKIT_GUI_EXPORT IQConstrainedSketchLayerListModel
    : public QAbstractTableModel
{
    Q_OBJECT

    std::set<std::string> layers_;
    IQVTKConstrainedSketchEditor *sketchEditor_;

public:
    explicit IQConstrainedSketchLayerListModel(IQVTKConstrainedSketchEditor *sk, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

Q_SIGNALS:
    void hideLayer(const std::string& layerName);
    void showLayer(const std::string& layerName);
    void renameLayer(const std::string& currentLayerName, const std::string& newLayerName);

public Q_SLOTS:
    void update();
};

#endif // IQCONSTRAINEDSKETCHLAYERLISTMODEL_H
