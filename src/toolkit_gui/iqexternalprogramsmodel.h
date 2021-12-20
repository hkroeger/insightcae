#ifndef IQEXTERNALPROGRAMSMODEL_H
#define IQEXTERNALPROGRAMSMODEL_H

#include <QAbstractItemModel>

#include "base/externalprograms.h"

class IQExternalProgramsModel : public QAbstractItemModel
{
    Q_OBJECT

    insight::ExternalPrograms externalPrograms_;

public:
    explicit IQExternalProgramsModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    insight::ExternalPrograms::iterator externalProgram(const QModelIndex& index);
    void resetProgramPath(const QModelIndex& index, const QString& newPath);

    const insight::ExternalPrograms& externalPrograms() const;

private:
};

#endif // IQEXTERNALPROGRAMSMODEL_H
