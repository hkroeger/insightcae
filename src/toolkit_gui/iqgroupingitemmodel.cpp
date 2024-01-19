#include "iqgroupingitemmodel.h"

#include "base/cppextensions.h"

#include "iqcaditemmodel.h"
#include "qnamespace.h"

#include <QDebug>
#include <QAbstractItemModel>




Node::Node(IQGroupingItemModel* model, Node *p, const QString &l, QPersistentModelIndex si)
    : QObject(p),
      model_(model),
      label(l),
      sourceIndex_(si)
{
    if (!sourceIndex_.isValid())
        isInsertedGroup_=true;
    else
    {
        isInsertedGroup_=false;
        model_->nodeMap_[sourceIndex_]=this;
    }

    if (auto *p = parentNode())
    {
        QModelIndex pi;
        if (auto *pp = p->parentNode())
        {
            pi=model_->createIndex(pp->children.indexOf(p), 0, p);
        }

        model_->beginInsertRows(pi, p->children.size(), p->children.size());
        parentNode()->children.push_back(this);
        model_->endInsertRows();
    }
}




Node::~Node()
{
    for (auto* cn: children)
    {
        delete cn;
    }

    Node* pn= parentNode();
    if (!pn)
    {
        pn=model_->rootNode_;
    }
    if (this!=model_->rootNode_)
    {
        QModelIndex myidx=
            model_->createIndex(pn->children.indexOf(this), 0, this);
        model_->beginRemoveRows(model_->parent(myidx), myidx.row(), myidx.row());
        pn->children.removeOne(this);
        model_->endRemoveRows();
    }

    auto j = std::find_if(
        model_->nodeMap_.begin(),
        model_->nodeMap_.end(),
        [this](const decltype(model_->nodeMap_)::value_type& mo)
            {return mo.second == this; }
    );

    if (j!=model_->nodeMap_.end())
    {
        model_->nodeMap_.erase(j);
    }
}




Node *Node::parentNode() const
{
    return static_cast<Node*>(parent());
}




QString Node::groupPath() const
{
    if (isInsertedGroup())
    {
        QString r;
        if (parentNode())
        {
            r=parentNode()->groupPath();
            if (!r.isEmpty()) r+="/";
        }
        r+=label;
        return r;
    }
    else
        return QString();
}




Node *Node::getOrCreateGroup(QStringList groupPath, SearchDirectionInHierarchy dir)
{

    if ( (dir==UpwardFirst) && isInsertedGroup() )
    {
        insight::assertion(
            parentNode()!=nullptr,
            "internal error in getOrCreateGroup: invalid parent node");

        // propagate request up first
        return parentNode()->getOrCreateGroup(groupPath);
    }
    else
    {
        // only look downward in hierarchy
        auto topGroupLabel = groupPath.first();

        auto childGroupPath = groupPath;
        childGroupPath.removeFirst();

        // matching child group?
        for (auto& c: children)
        {
            if (c->label==topGroupLabel)
            {
                if (childGroupPath.size()>0)
                    return c->getOrCreateGroup(childGroupPath, OnlyDownward);
                else
                    return c;
            }
        }

        // no match, create child group
        auto cn=new Node(model_, this, topGroupLabel);
        if (childGroupPath.size()>0)
            return cn->getOrCreateGroup(childGroupPath, OnlyDownward);
        else
            return cn;
    }
}




QPersistentModelIndex Node::sourceIndex(int column) const
{
    return sourceIndex_.sibling(sourceIndex_.row(), column);
}




bool Node::isMappedToSource() const
{
    return sourceIndex_.isValid();
}




bool Node::isInsertedGroup() const
{
    return isInsertedGroup_;
}




void IQGroupingItemModel::decorateSourceNode(
    const QModelIndex& sourceIndex, Node* parent )
{
    QString label =
        sourceModel()->data(
             sourceIndex.siblingAtColumn(labelCol_)
        ).toString();

    insight::assertion(
        parent!=nullptr,
        "internal error in decorateSourceNode: invalid parent node");

    if (parent)
    {
        auto pp=parent->groupPath();
        if (label.startsWith(pp))
            label.remove(0, pp.size());
        if (label.startsWith("/"))
            label.remove(0, 1);
    }
    auto groups = label.split('/');
    groups.removeLast(); // last is item name, not group


    if (groups.size()>0)
    {
        // insert split group
        auto ng= parent->getOrCreateGroup(groups);
        decorateSourceNode(sourceIndex, ng);
    }
    else
    {
        // keep as is
        auto *ng = new Node(this, parent, label, sourceIndex);
        // decorate children also
        int nc=sourceModel()->rowCount(sourceIndex);
        for (int i=0; i<nc; ++i)
        {
            decorateSourceNode(sourceModel()->index(i, 0, sourceIndex), ng);
        }
    }
}




IQGroupingItemModel::IQGroupingItemModel(QObject *parent)
    : //QAbstractProxyModel{parent},
    QAbstractItemModel(parent),
    sourceModel_(nullptr),
    labelCol_(0)
{
    rootNode_ = new Node(this, nullptr, "");
}




IQGroupingItemModel::~IQGroupingItemModel()
{
    delete rootNode_;
}




void IQGroupingItemModel::setGroupColumn(int labelColumn)
{
    labelCol_=labelColumn;
}




QAbstractItemModel *IQGroupingItemModel::sourceModel() const
{
    return sourceModel_;
}




void IQGroupingItemModel::setSourceModel(QAbstractItemModel *sm)
{
    rootNode_->children.clear();

    sourceModel_=sm;

    int n=sm->rowCount();
    for (int i=0; i<n; ++i)
    {
        decorateSourceNode(sm->index(i, 0), rootNode_);
    }

#warning needs to be disconnected on source model change

    connect(sm, &QAbstractItemModel::rowsInserted, this,
            [&](const QModelIndex &sourceParent, int first, int last)
            {
                auto psi=mapFromSource(sourceParent);
                // if (!psi.isValid())
                // {
                //     qDebug() <<sourceModel()->data(
                //         sourceParent.siblingAtColumn(IQCADItemModel::labelCol));
                // }
                for (int i=first; i<=last; ++i)
                {
                    decorateSourceNode(
                        sourceModel()->index(i, 0, sourceParent),
                        static_cast<Node*>(psi.internalPointer())
                        );
                }
            }
            );

    connect(sm, &QAbstractItemModel::rowsAboutToBeRemoved, this,
            [&](const QModelIndex& spidx, int first, int last)
            {
                for (int row=last; row>=first; row--)
                {
                    auto i = mapFromSource(
                        sourceModel()->index(row, 0, spidx) );

                    if (i.isValid())
                    {
                        auto ip = parent(i);

                        // remove mapped item
                        auto n=static_cast<Node*>(i.internalPointer());
                        delete n;

                        // remove parent groups, if required
                        std::function<void(const QModelIndex&)> removeParentNodeIfRequired;

                        removeParentNodeIfRequired=
                            [this,&removeParentNodeIfRequired]
                            (const QModelIndex& i)
                            {
                                if (i.isValid())
                                {
                                    auto pn=static_cast<Node*>(i.internalPointer());
                                    if (!pn->isMappedToSource())
                                    {
                                        auto ip=parent(i);
                                        if (pn->children.empty())
                                        {
                                            delete pn;
                                            removeParentNodeIfRequired(ip);
                                        }
                                    }
                                }
                            };

                        removeParentNodeIfRequired(ip);
                    }
                }
            }
            );


    connect(sm, &QAbstractItemModel::dataChanged, this,
            [&](const QModelIndex &topLeft,
                const QModelIndex &bottomRight,
                const QVector<int> &roles )
            {
                insight::assertion(
                    topLeft.parent()==bottomRight.parent(),
                    "unexpected");

                auto tls=mapFromSource(topLeft);
                Q_EMIT dataChanged(
                    tls,
                    mapFromSource(bottomRight),
                    roles
                    );

                if (roles.contains(Qt::CheckStateRole))
                {
                    std::function<void(const QModelIndex&)> updateParents;
                    updateParents=
                        [this,&roles,&updateParents](const QModelIndex& idx)
                        {
                            auto pi=parent(idx);
                            if (auto pn=static_cast<Node*>(pi.internalPointer()))
                            {
                                // go up in grouping hierarchy and update checkboxes
                                if (!pn->isMappedToSource())
                                {
                                    Q_EMIT dataChanged(pi, pi, roles);
                                    updateParents(pi);
                                }
                            }
                        };

                    updateParents(tls);
                }
            }
            );
}




QModelIndex IQGroupingItemModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    for (auto& nm: nodeMap_)
    {
        if (nm.first==sourceIndex)
        {
            auto *n=nm.second;
            auto row=n->parentNode()->children.indexOf(n);
            return createIndex(row, sourceIndex.column(), n);
        }
    }
    return QModelIndex();
}




QModelIndex IQGroupingItemModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (auto* n = static_cast<Node*>(proxyIndex.internalPointer()))
    {
        return n->sourceIndex(proxyIndex.column());
    }
    return QModelIndex();
}




QModelIndex IQGroupingItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        // top node
        if (row>=0 && row < rootNode_->children.size())
        {
            return createIndex(row, column, rootNode_->children.at(row));
        }
    }
    else if (auto* p = static_cast<Node*>(parent.internalPointer()))
    {
        if (row>=0 && row < p->children.size())
        {
            return createIndex(row, column, p->children.at(row));
        }
    }

    return QModelIndex();
}




QModelIndex IQGroupingItemModel::parent(const QModelIndex &index) const
{
    if ( index.isValid() )
    {
        if (auto* p=static_cast<Node*>(index.internalPointer()))
        {
            auto* pp = p->parentNode();
            if (auto* ppp = pp->parentNode())
            {
                return createIndex(ppp->children.indexOf(pp), 0, pp);
            }
        }
    }

    return QModelIndex();
}




int IQGroupingItemModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        return rootNode_->children.size();
    }
    else if (auto *n=static_cast<Node*>(parent.internalPointer()))
    {
        if (parent.column()==0)
        {
            return n->children.size();
        }
    }
    return 0;
}




int IQGroupingItemModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        return sourceModel()->columnCount();
    }
    else if (auto *n=static_cast<Node*>(parent.internalPointer()))
    {
        return sourceModel()->columnCount(
            n->sourceIndex(parent.column())
            );
    }
    return 0;
}




QVariant IQGroupingItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return sourceModel()->headerData(section, orientation, role);
}




QVariant IQGroupingItemModel::data(const QModelIndex &index, int role) const
{
    if (auto *n=static_cast<Node*>(index.internalPointer()))
    {
        if (n->isMappedToSource())
        {
            if (index.column()==labelCol_ && role==Qt::DisplayRole)
            {
                return n->label;
            }
            else
            {
                return sourceModel()->data(n->sourceIndex(index.column()), role);
            }
        }
        else
        {
            if (role==Qt::DisplayRole && index.column()==1)
            {
                return n->label;
            }
            if (role == Qt::CheckStateRole)
            {
                int nChecked=0, nUnchecked=0;
                loopOverMappedChildren(
                    n, index.column(),
                    [&](QModelIndex sourceIndex)
                    {
                        auto d=sourceModel()->data(
                            sourceIndex, role );
                        if (d.canConvert<Qt::CheckState>())
                        {
                            auto cst = d.value<Qt::CheckState>();
                            if (cst==Qt::Checked) nChecked++;
                            if (cst==Qt::Unchecked) nUnchecked++;
                        }
                        return true;
                    }
                );

                if (nChecked==0 && nUnchecked>0) return Qt::Unchecked;
                if (nChecked>0 && nUnchecked==0) return Qt::Checked;
                if (nChecked>0 && nUnchecked>0) return Qt::PartiallyChecked;
            }
        }
    }
    return QVariant();
}




bool IQGroupingItemModel::loopOverMappedChildren(
    Node *n, int column,
    std::function<bool(QModelIndex)> body ) const
{
    bool result = false;
    for (auto& c: n->children)
    {
        bool thisSuccess=true, childrenSuccess;
        if (c->isMappedToSource())
        {
            thisSuccess = body(c->sourceIndex( column ) );
        }
        childrenSuccess = loopOverMappedChildren(c, column, body);
        result = result || thisSuccess || childrenSuccess;
    }
    return result;
}




Qt::ItemFlags IQGroupingItemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return 0;
    }

    if (auto *n=static_cast<Node*>(index.internalPointer()))
    {
        if (n->isMappedToSource())
        {
            return sourceModel()->flags(
                n->sourceIndex(index.column()));
        }
        else
        {
            auto flags=QAbstractItemModel::flags(index);
            // loop through children
            bool someChildIsCheckable=
                loopOverMappedChildren(
                n, index.column(),
                [&](QModelIndex sourceIndex)
                {
                    auto f = sourceModel()->flags(sourceIndex);
                    return bool( f & Qt::ItemIsUserCheckable );
                }
            );
            if (someChildIsCheckable)
            {
                flags|=Qt::ItemIsUserCheckable;
            }
            return flags;
        }
    }

    return QAbstractItemModel::flags(index);
}




bool IQGroupingItemModel::setData(
    const QModelIndex &index,
    const QVariant &value,
    int role )
{
    if (auto *n=static_cast<Node*>(index.internalPointer()))
    {
        if (n->isMappedToSource())
        {
            return sourceModel()->setData(
                n->sourceIndex(
                    index.column()), value, role);
        }
        else if (role == Qt::CheckStateRole)
        {
            if (value.canConvert<Qt::CheckState>())
            {
                auto ncs = value.value<Qt::CheckState>();

                return loopOverMappedChildren(
                    n, index.column(),
                    [&](QModelIndex srcIdx)
                    {
                        return sourceModel()->setData(srcIdx, ncs, role);
                    }
                );
            }
        }
    }
    return false;
}



