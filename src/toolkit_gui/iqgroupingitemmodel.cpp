#include "iqgroupingitemmodel.h"

#include "base/cppextensions.h"

#include "iqcaditemmodel.h"

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
    labelCol_(0),
    rootNode_(this, nullptr, "")
{}

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
    rootNode_.children.clear();

//    QAbstractProxyModel::setSourceModel(sourceModel);
    sourceModel_=sm;

    int n=sm->rowCount();
    for (int i=0; i<n; ++i)
    {
        decorateSourceNode(sm->index(i, 0), &rootNode_);
    }

    connect(sm, &QAbstractItemModel::rowsInserted, this,
            [&](const QModelIndex &sourceParent, int first, int last)
            {
                qDebug()<<"sp="<<sourceParent;
                auto psi=mapFromSource(sourceParent);
                if (!psi.isValid())
                {
                    qDebug() <<sourceModel()->data(
                        sourceParent.siblingAtColumn(IQCADItemModel::labelCol));
                }
                for (int i=first; i<=last; ++i)
                {
                    decorateSourceNode(
                        sourceModel()->index(i, 0, sourceParent),
                        static_cast<Node*>(psi.internalPointer())
                        );
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



//QModelIndex IQGroupingItemModel::mapFromSource(const QModelIndex &sourceIndex) const
//{
//    return QModelIndex();
//}



//QModelIndex IQGroupingItemModel::mapToSource(const QModelIndex &proxyIndex) const
//{
//    if (auto *n=static_cast<Node*>(proxyIndex.internalPointer()))
//    {
//        return n->sourceIndex(proxyIndex.column());
//    }
//    return QModelIndex();
//}


QModelIndex IQGroupingItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        // top node
        if (row>=0 && row < rootNode_.children.size())
        {
            return createIndex(row, column, rootNode_.children.at(row));
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
        return rootNode_.children.size();
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
        }
    }
    return QVariant();
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
            return sourceModel()->flags(n->sourceIndex(index.column()));
    }

    return QAbstractItemModel::flags(index);
}

bool IQGroupingItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (auto *n=static_cast<Node*>(index.internalPointer()))
    {
        if (n->isMappedToSource())
            return sourceModel()->setData(n->sourceIndex(index.column()), value, role);
    }
    return false;
}



