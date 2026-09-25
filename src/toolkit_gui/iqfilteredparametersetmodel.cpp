#include "iqfilteredparametersetmodel.h"

#include "base/elementpath.h"

#include <algorithm>




void IQFilteredParameterSetModel::searchRootSourceIndices(
    QAbstractItemModel *sourceModel,
    const QModelIndex& parent )
{
    for (int r=0; r<sourceModel->rowCount(parent); ++r)
    {
        auto i=sourceModel->index(r, IQParameterSetModel::stringPathCol, parent);
        insight::ElementPath curPath( sourceModel->data(i).toString().toStdString() );

        bool searchDown=false;
        bool addThis=false;

        for (auto& sp: qAsConst(sourceRootParameterPaths_))
        {
            insight::ElementPath sourceParam(sp);
            bool wildcard=false;
            if (!sourceParam.empty() && sourceParam.back()=="*")
            {
                wildcard=true;
                sourceParam.pop_back();
            }

            if (wildcard)
            {
                if ( curPath.size()==sourceParam.size()+1
                     && curPath.isBelow(sourceParam) )
                {
                    // curPath is a direct child of the wildcard prefix: add as root
                    addThis=true;
                }
                else if ( curPath.size()<=sourceParam.size()
                          && sourceParam.isBelow(curPath) )
                {
                    // curPath is an ancestor of (or equal to) the wildcard prefix:
                    // descend to find the direct children
                    searchDown=true;
                }
            }
            else
            {
                if ( curPath.size()==sourceParam.size()
                     && curPath.isBelow(sourceParam) )
                {
                    // exact match
                    addThis=true;
                }
                else if ( curPath.size()<sourceParam.size()
                          && sourceParam.isBelow(curPath) )
                {
                    // curPath is an ancestor of sourceParam: descend to find it
                    searchDown=true;
                }
            }
        }

        if (addThis)
            rootSourceIndices.append(i.siblingAtColumn(0));

        if (searchDown && !addThis)
            searchRootSourceIndices(sourceModel, i.siblingAtColumn(0));
    }
}




bool IQFilteredParameterSetModel::isBelowRootParameter(const QModelIndex &sourceIndex_, int* topRow) const
{
    if (!sourceIndex_.isValid())
        return false;

    // root indices are stored for the label column
    auto sourceIndex = sourceIndex_.siblingAtColumn(0);

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




const QPersistentModelIndex*
IQFilteredParameterSetModel::storedSourceParent(const QModelIndex &sourceParent) const
{
    if (!sourceParent.isValid())
        return nullptr;

    auto sp = sourceParent.siblingAtColumn(0);

    auto i = std::find(sourceParents_.begin(), sourceParents_.end(), sp);
    if (i!=sourceParents_.end())
        return &(*i);

    sourceParents_.push_back(QPersistentModelIndex(sp));
    return &sourceParents_.back();
}




bool IQFilteredParameterSetModel::affectsRootParameters(const QModelIndex &sourceParent) const
{
    insight::ElementPath parentPath;
    if (sourceParent.isValid())
    {
        parentPath = insight::ElementPath(
            sourceModel()->data(
                sourceParent.siblingAtColumn(IQParameterSetModel::stringPathCol) )
                .toString().toStdString() );
    }

    for (auto& sp: sourceRootParameterPaths_)
    {
        insight::ElementPath sourceParam(sp);
        bool wildcard=false;
        if (!sourceParam.empty() && sourceParam.back()=="*")
        {
            wildcard=true;
            sourceParam.pop_back();
        }

        // the children of sourceParent contain a root parameter or one of its ancestors
        if ( ( sourceParam.size()>parentPath.size()
               || (wildcard && sourceParam.size()==parentPath.size()) )
             && sourceParam.isBelow(parentPath) )
        {
            return true;
        }
    }

    return false;
}




void IQFilteredParameterSetModel::beginReset()
{
    beginResetModel();
    pendingChange_=PendingChange::Reset;
}




void IQFilteredParameterSetModel::endReset()
{
    rootSourceIndices.clear();
    sourceParents_.clear();
    searchRootSourceIndices(sourceModel(), QModelIndex());
    endResetModel();
    pendingChange_=PendingChange::None;
}




void IQFilteredParameterSetModel::connectToSourceModel(QAbstractItemModel *sourceModel)
{
    connect(sourceModel, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
            {
                if (pendingChange_==PendingChange::Reset)
                    return;

                if (isBelowRootParameter(topLeft)&&isBelowRootParameter(bottomRight))
                    Q_EMIT dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight), roles);
            }
            );

    auto finishChange = [this]()
    {
        switch (pendingChange_)
        {
        case PendingChange::Insert: endInsertRows(); break;
        case PendingChange::Remove: endRemoveRows(); break;
        case PendingChange::Move: endMoveRows(); break;
        case PendingChange::Reset: endReset(); break;
        case PendingChange::None: break;
        }
        pendingChange_=PendingChange::None;
    };

    connect(sourceModel, &QAbstractItemModel::rowsAboutToBeInserted, this,
            [this](const QModelIndex &parent, int first, int last)
            {
                if (isBelowRootParameter(parent))
                {
                    beginInsertRows(mapFromSource(parent), first, last);
                    pendingChange_=PendingChange::Insert;
                }
                else if (affectsRootParameters(parent))
                {
                    beginReset();
                }
            }
            );
    connect(sourceModel, &QAbstractItemModel::rowsInserted, this, finishChange);

    connect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this,
            [this](const QModelIndex &parent, int first, int last)
            {
                if (isBelowRootParameter(parent))
                {
                    beginRemoveRows(mapFromSource(parent), first, last);
                    pendingChange_=PendingChange::Remove;
                }
                else if (affectsRootParameters(parent))
                {
                    beginReset();
                }
            }
            );
    connect(sourceModel, &QAbstractItemModel::rowsRemoved, this, finishChange);

    connect(sourceModel, &QAbstractItemModel::rowsAboutToBeMoved, this,
            [this](const QModelIndex &sourceParent, int sourceFirst, int sourceLast,
                   const QModelIndex &destinationParent, int destinationRow)
            {
                bool srcMapped=isBelowRootParameter(sourceParent);
                bool dstMapped=isBelowRootParameter(destinationParent);

                if (srcMapped && dstMapped
                    && beginMoveRows(
                        mapFromSource(sourceParent), sourceFirst, sourceLast,
                        mapFromSource(destinationParent), destinationRow ) )
                {
                    pendingChange_=PendingChange::Move;
                }
                else if ( srcMapped || dstMapped
                         || affectsRootParameters(sourceParent)
                         || affectsRootParameters(destinationParent) )
                {
                    beginReset();
                }
            }
            );
    connect(sourceModel, &QAbstractItemModel::rowsMoved, this, finishChange);

    connect(sourceModel, &QAbstractItemModel::modelAboutToBeReset, this,
            [this]() { beginReset(); } );
    connect(sourceModel, &QAbstractItemModel::modelReset, this, finishChange);

    connect(sourceModel, &QAbstractItemModel::layoutAboutToBeChanged, this,
            [this]() { beginReset(); } );
    connect(sourceModel, &QAbstractItemModel::layoutChanged, this, finishChange);
}




IQFilteredParameterSetModel::IQFilteredParameterSetModel(
    const std::vector<std::string>& sourceParameterPaths,
    QObject *parent )
    : QAbstractProxyModel(parent),
    sourceRootParameterPaths_(sourceParameterPaths)
{}




void IQFilteredParameterSetModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    beginResetModel();

    // drop connections to previous source model
    // (before the base class reconnects its own)
    if (this->sourceModel())
        disconnect(this->sourceModel(), nullptr, this, nullptr);

    rootSourceIndices.clear();
    sourceParents_.clear();
    pendingChange_=PendingChange::None;

    QAbstractProxyModel::setSourceModel(sourceModel);

    if (sourceModel)
    {
        searchRootSourceIndices(sourceModel, QModelIndex());
        connectToSourceModel(sourceModel);
    }

    endResetModel();
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
        auto& sppi = *static_cast<const QPersistentModelIndex*>(
            proxyIndex.internalPointer() );
        return sourceModel()->index(proxyIndex.row(), proxyIndex.column(), sppi);
    }
    return QModelIndex();
}




int IQFilteredParameterSetModel::columnCount(const QModelIndex &parent) const
{
    if (!sourceModel())
        return 0;

    return sourceModel()->columnCount(mapToSource(parent));
}




int IQFilteredParameterSetModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        return rootSourceIndices.size();
    }
    else
    {
        return sourceModel()->rowCount(mapToSource(parent));
    }
}




QModelIndex IQFilteredParameterSetModel::index(int row, int column, const QModelIndex &parent) const
{
    // don't use hasIndex(): the hidden columns (stringPathCol, iqParamCol)
    // are beyond columnCount() but need to be accessible
    if (row<0 || column<0 || row>=rowCount(parent))
        return QModelIndex();

    if (!parent.isValid())
    {
        // top rows
        return createIndex(row, column, nullptr);
    }
    else
    {
        // below top rows: store the source parent
        if (auto *sp = storedSourceParent(mapToSource(parent)))
        {
            return createIndex(
                row, column,
                const_cast<void*>(static_cast<const void*>(sp)) );
        }
    }

    return QModelIndex();
}




QModelIndex IQFilteredParameterSetModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || !index.internalPointer())
    {
        // top level
        return QModelIndex();
    }

    auto& sppi = *static_cast<const QPersistentModelIndex*>(
        index.internalPointer() );

    return mapFromSource(sppi);
}
