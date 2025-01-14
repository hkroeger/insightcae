#ifndef IQFILTEREDPARAMETERSETMODEL_H
#define IQFILTEREDPARAMETERSETMODEL_H


#include "iqparametersetmodel.h"


class TOOLKIT_GUI_EXPORT IQFilteredParameterSetModel
    : public QAbstractProxyModel
{


    std::vector<std::string> sourceRootParameterPaths_;
    QList<QPersistentModelIndex> rootSourceIndices;

    QList<QPersistentModelIndex> mappedIndices_;
    void searchRootSourceIndices(QAbstractItemModel *sourceModel, const QModelIndex& sourceParent);
    void storeAllChildSourceIndices(QAbstractItemModel *sourceModel, const QModelIndex& sourceParent);
    bool isBelowRootParameter(const QModelIndex& sourceIndex, int* topRow=nullptr) const;

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
