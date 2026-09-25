#ifndef IQFILTEREDPARAMETERSETMODEL_H
#define IQFILTEREDPARAMETERSETMODEL_H


#include "iqparametersetmodel.h"

#include <deque>


class TOOLKIT_GUI_EXPORT IQFilteredParameterSetModel
    : public QAbstractProxyModel
{

    std::vector<std::string> sourceRootParameterPaths_;
    QList<QPersistentModelIndex> rootSourceIndices;

    // source parents of all proxy indices below top level.
    // Proxy indices store a pointer to an entry as internal pointer,
    // so the container must not relocate its elements on insertion (-> deque).
    mutable std::deque<QPersistentModelIndex> sourceParents_;

    // structural change of the source model, which is currently
    // in progress and has been forwarded (begin* called)
    enum class PendingChange { None, Insert, Remove, Move, Reset };
    PendingChange pendingChange_ = PendingChange::None;

    void searchRootSourceIndices(QAbstractItemModel *sourceModel, const QModelIndex& sourceParent);
    bool isBelowRootParameter(const QModelIndex& sourceIndex, int* topRow=nullptr) const;
    const QPersistentModelIndex* storedSourceParent(const QModelIndex& sourceParent) const;

    /**
     * @brief affectsRootParameters
     * check, if a structural change of the children of sourceParent
     * might change the set of root parameters
     */
    bool affectsRootParameters(const QModelIndex& sourceParent) const;

    void beginReset();
    void endReset();

    void connectToSourceModel(QAbstractItemModel *sourceModel);

public:
    IQFilteredParameterSetModel(const std::vector<std::string>& sourceParameterPaths, QObject* parent=nullptr);

    void setSourceModel(QAbstractItemModel *sourceModel) override;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;

    int	columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int	rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex	index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex	parent(const QModelIndex &index) const override;

};

#endif // IQFILTEREDPARAMETERSETMODEL_H
