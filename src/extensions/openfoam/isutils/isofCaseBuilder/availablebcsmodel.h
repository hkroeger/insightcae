#ifndef AVAILABLEBCSMODEL_H
#define AVAILABLEBCSMODEL_H

#include <QAbstractListModel>

class AvailableBCsModel : public QAbstractListModel
{
  Q_OBJECT

  QList<std::string> BCtypes_;

public:
  explicit AvailableBCsModel(QObject *parent = nullptr);

  // Header:
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  // Basic functionality:
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  std::string selectedBCType(const QModelIndex& index) const;
};

#endif // AVAILABLEBCSMODEL_H
