#ifndef IQSUPPLEMENTEDINPUTDATAMODEL_H
#define IQSUPPLEMENTEDINPUTDATAMODEL_H

#include "toolkit_gui_export.h"

#include <QAbstractTableModel>

#include "base/supplementedinputdata.h"

class TOOLKIT_GUI_EXPORT IQSupplementedInputDataModel
    : public QAbstractTableModel
{
  Q_OBJECT

  insight::supplementedInputDataBase::ReportedSupplementQuantitiesTable reportedSupplementQuantities_;

public:
  explicit IQSupplementedInputDataModel(QObject *parent = nullptr);

  // Header:
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  // Basic functionality:
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  void reset(const insight::supplementedInputDataBase::ReportedSupplementQuantitiesTable& data);
};

#endif // IQSUPPLEMENTEDINPUTDATAMODEL_H
