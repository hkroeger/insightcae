#ifndef IQGROUPINGITEMMODEL_H
#define IQGROUPINGITEMMODEL_H

#include <QAbstractProxyModel>

#include "base/boost_include.h"


class Node;
class IQGroupingItemModel;

typedef QList<Node*> ChildrenList;

class Node
: public QObject
{
    Q_OBJECT

    IQGroupingItemModel* model_;

    /**
     * @brief sourceIndex
     * mapped index, might be invalid, if this an inserted group
     */
    QPersistentModelIndex sourceIndex_;

    bool isInsertedGroup_;

public:
    QString label;

    ChildrenList children;

    Node(IQGroupingItemModel* model, Node* parent, const QString& label, QPersistentModelIndex sourceIndex = QPersistentModelIndex());
    ~Node();

    Node* parentNode() const;

    QString groupPath() const;

    enum SearchDirectionInHierarchy { UpwardFirst, OnlyDownward };
    Node* getOrCreateGroup(QStringList groupPath, SearchDirectionInHierarchy dir=UpwardFirst);

    QPersistentModelIndex sourceIndex(int column) const;
    bool isMappedToSource() const;
    bool isInsertedGroup() const;
};


class IQGroupingItemModel
    : public QAbstractItemModel
{
    friend class Node;

    QAbstractItemModel *sourceModel_;
    int labelCol_;

    Node *rootNode_;
    std::map<QPersistentModelIndex, Node*> nodeMap_;

    void decorateSourceNode(const QModelIndex& sourceIndex, Node* parent);

    bool loopOverMappedChildren(
        Node *n, int column,
        std::function<bool(QModelIndex)> body ) const;

public:
    explicit IQGroupingItemModel(QObject *parent = nullptr);
    ~IQGroupingItemModel();

    void setGroupColumn(int labelColumn);

    QAbstractItemModel *sourceModel() const;
    virtual void setSourceModel(QAbstractItemModel *sourceModel) /*override*/;

    QModelIndex mapFromSource (const QModelIndex &sourceIndex) const;
    QModelIndex mapToSource (const QModelIndex &proxyIndex) const;


    // Basic functionality:
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
};

#endif // IQGROUPINGITEMMODEL_H
