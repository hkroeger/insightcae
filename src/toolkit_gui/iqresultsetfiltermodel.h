#ifndef IQRESULTSETFILTERMODEL_H
#define IQRESULTSETFILTERMODEL_H

#include <QAbstractListModel>
#include "base/hierarchicaldatafilter.h"


class IQResultSetFilterModel : public QAbstractListModel
{
    Q_OBJECT

    insight::hierarchicalData::Filter rsf_;

public:
    explicit IQResultSetFilterModel(
            QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    void resetFilter(const insight::hierarchicalData::Filter& rsf);
    void clear();
    const insight::hierarchicalData::Filter& filter() const;
};

#endif // IQRESULTSETFILTERMODEL_H
