#ifndef IQFILTEREDRESULTSETMODEL_H
#define IQFILTEREDRESULTSETMODEL_H

#include "iqresultsetmodel.h"
#include "iqhierarchicaldatafilterproxymodel.h"

namespace insight
{


class IQFilteredResultSetModel
    : public IQHierarchicalDataFilterProxyModel,
      public IQResultSetModelBase
{
    Q_OBJECT

public:
    IQFilteredResultSetModel(QObject *parent = 0);

    IQResultElement* getResultElement(const QModelIndex& idx) override;

    // void addChildren(const QModelIndex& pidx, insight::ResultElementCollection* re) const;
};

}

#endif // IQFILTEREDRESULTSETMODEL_H
