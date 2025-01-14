#include "iqfilteredparametersetmodel.h"




void IQFilteredParameterSetModel::searchRootSourceIndices(
    QAbstractItemModel *sourceModel,
    const QModelIndex& parent )
{
    for (int r=0; r<sourceModel->rowCount(parent); ++r)
    {
        auto i=sourceModel->index(r, IQParameterSetModel::stringPathCol, parent);
        auto curPath=sourceModel->data(i).toString();

        bool searchDown=false;
        for (const auto& sourceParam: qAsConst(sourceRootParameterPaths_))
        {
            if (boost::ends_with(sourceParam, "/*") // support wildcard to select all below
                &&  boost::starts_with(curPath, sourceParam.substr(0, sourceParam.size()-2))
                &&  curPath.size()>sourceParam.size()-2 )
            {
                rootSourceIndices.append(i.siblingAtColumn(0));
                searchDown=false;
            }
            else if (boost::starts_with(sourceParam, curPath)) // sourceParam starts with curPath?
            {
                if (curPath.size()==sourceParam.size())
                {
                    rootSourceIndices.append(i.siblingAtColumn(0));
                }
                else if (curPath.size()<sourceParam.size())
                {
                    searchDown=true;
                }
            }
        }

        if (searchDown)
        {
            searchRootSourceIndices(sourceModel, i.siblingAtColumn(0));
        }

    }
}




void IQFilteredParameterSetModel::storeAllChildSourceIndices(QAbstractItemModel *sourceModel, const QModelIndex &sourceIndex)
{
    if (!mappedIndices_.contains(sourceIndex))
        mappedIndices_.append(sourceIndex);

    for (int r=0; r<sourceModel->rowCount(sourceIndex); ++r)
    {
        storeAllChildSourceIndices(
            sourceModel,
            sourceModel->index(r, 0, sourceIndex) );
    }
}




bool IQFilteredParameterSetModel::isBelowRootParameter(const QModelIndex &sourceIndex, int* topRow) const
{
    if (!sourceIndex.isValid())
        return false;

    if (rootSourceIndices.contains(sourceIndex))
    {
        if (topRow)
            *topRow=rootSourceIndices.indexOf(sourceIndex);
        return true;
    }

    auto sourceIndexParent = sourceModel()->parent(sourceIndex);

    if (!sourceIndexParent.isValid())
    {
        return false;
    }
    else
    {
        if (rootSourceIndices.contains(sourceIndexParent))
        {
            if (topRow) *topRow=-1;
            return true;
        }
        else
            return isBelowRootParameter(sourceIndexParent, topRow);
    }
}




IQFilteredParameterSetModel::IQFilteredParameterSetModel(
    const std::vector<std::string>& sourceParameterPaths,
    QObject *parent )
    : QAbstractProxyModel(parent),
    sourceRootParameterPaths_(sourceParameterPaths)
{}




void IQFilteredParameterSetModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    searchRootSourceIndices(sourceModel, QModelIndex());
    for (auto& pi: qAsConst(rootSourceIndices))
    {
        storeAllChildSourceIndices(sourceModel, pi);
    }
    QAbstractProxyModel::setSourceModel(sourceModel);
    connect(sourceModel, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
            {
                if (isBelowRootParameter(topLeft)&&isBelowRootParameter(bottomRight))
                    Q_EMIT dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight), roles);
            }
            );
#warning needs disconnect somewhere
}




QModelIndex IQFilteredParameterSetModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid())
        return QModelIndex();

    int topRow=-1;
    if (isBelowRootParameter(sourceIndex, &topRow))
    {
        if (topRow>=0)
        {
            return index(topRow, sourceIndex.column());
        }
        else
        {
            return index(sourceIndex.row(), sourceIndex.column(), mapFromSource(sourceIndex.parent()));
        }
    }
    return QModelIndex();
}




QModelIndex IQFilteredParameterSetModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid())
    {
        return QModelIndex();
    }

    if (!proxyIndex.internalPointer())  // top level
    {
        if (proxyIndex.row()>=0 && proxyIndex.row()<rootSourceIndices.size())
        {
            return QModelIndex(rootSourceIndices[proxyIndex.row()]).siblingAtColumn(proxyIndex.column());
        }
    }
    else
    {
        auto& sppi = *reinterpret_cast<QPersistentModelIndex*>(
            proxyIndex.internalPointer() );
        return sourceModel()->index(proxyIndex.row(), proxyIndex.column(), sppi);
    }
    return QModelIndex();
}




int IQFilteredParameterSetModel::columnCount(const QModelIndex &parent) const
{
    auto nc = sourceModel()->columnCount(mapToSource(parent));
    return nc;
}




int IQFilteredParameterSetModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        return rootSourceIndices.size();
    }
    else
    {
        auto r= sourceModel()->rowCount(mapToSource(parent));
        return r;
    }

    return 0;
}




QModelIndex IQFilteredParameterSetModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        // top rows
        if (row>=0 && row<rootSourceIndices.size())
        {
            return createIndex(
                row, column, nullptr
                );
        }
    }
    else
    {
        auto pp=parent.parent();
        if (!pp.isValid())
        {
            // one below top rows
            auto mi=mappedIndices_.indexOf(rootSourceIndices[parent.row()]);
            return createIndex(
                row, column,
                const_cast<void*>(reinterpret_cast<const void*>(&mappedIndices_[mi]))
                );
        }
        else
        {
            auto si=mapToSource(parent);
            auto mi=mappedIndices_.indexOf(si);
            // more than one level below top rows
            return createIndex(
                row, column,
                //parent.internalPointer()
                const_cast<void*>(reinterpret_cast<const void*>(&mappedIndices_[mi]))
                );
        }
    }

    return QModelIndex();
}




QModelIndex IQFilteredParameterSetModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        // top node
        return QModelIndex();
    }

    if (index.internalPointer())
    {
        // some index below top level
        auto& sppi = *reinterpret_cast<QPersistentModelIndex*>(
            index.internalPointer() );

        auto mi1=rootSourceIndices.indexOf(sppi);
        if (mi1>=0)
        {
            // top level row
            return createIndex(
                mi1, 0,
                nullptr
                );
        }
        else
        {
            // below top level row
            auto mi=mappedIndices_.indexOf(sppi.parent());
            return createIndex(
                sppi.row(), 0,
                const_cast<void*>(reinterpret_cast<const void*>(&mappedIndices_[mi]))
                );
        }
    }

    return QModelIndex();
}

