#ifndef IQREPORTTEMPLATESMODEL_H
#define IQREPORTTEMPLATESMODEL_H

#include <QAbstractItemModel>

#include "base/resultreporttemplates.h"

class IQReportTemplatesModel : public QAbstractItemModel
{
    Q_OBJECT

    insight::ResultReportTemplates templates_;

public:
    explicit IQReportTemplatesModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void addTemplate(const QString& filePath);
    void setDefaultTemplate(const QModelIndex& index);
    void removeTemplate(const QModelIndex& index);

private:
};

#endif // IQREPORTTEMPLATESMODEL_H
