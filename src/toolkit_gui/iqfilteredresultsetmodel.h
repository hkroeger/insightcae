#ifndef IQFILTEREDRESULTSETMODEL_H
#define IQFILTEREDRESULTSETMODEL_H

#include "iqresultsetmodel.h"

namespace insight
{


class IQFilteredResultSetModel
    : public QSortFilterProxyModel,
      public IQResultSetModelBase
{
    Q_OBJECT

    hierarchicalData::Filter filter_;

public:
    IQFilteredResultSetModel(QObject *parent = 0);

    IQResultElement* getResultElement(const QModelIndex& idx) override;

    const hierarchicalData::Filter& filter() const;
    void resetFilter(const insight::hierarchicalData::Filter& filter);

    // void addChildren(const QModelIndex& pidx, insight::ResultElementCollection* re) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

}

#endif // IQFILTEREDRESULTSETMODEL_H
