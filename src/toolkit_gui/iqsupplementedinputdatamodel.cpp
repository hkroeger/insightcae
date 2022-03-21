#include "iqsupplementedinputdatamodel.h"

IQSupplementedInputDataModel::IQSupplementedInputDataModel(QObject *parent)
  : QAbstractTableModel(parent)
{}

QVariant IQSupplementedInputDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation==Qt::Horizontal && role==Qt::DisplayRole)
  {
    switch (section)
    {
      case 0: return "Name";
      case 1: return "Value";
      case 2: return "Unit";
      case 3: return "Description";
    }
  }
  return QVariant();
}

int IQSupplementedInputDataModel::rowCount(const QModelIndex &parent) const
{
  if (parent.isValid())
    return 0;

  return reportedSupplementQuantities_.size();
}

int IQSupplementedInputDataModel::columnCount(const QModelIndex &parent) const
{
  if (parent.isValid())
    return 0;

  return 4;
}

QVariant IQSupplementedInputDataModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (role==Qt::DisplayRole)
  {
    auto i = reportedSupplementQuantities_.begin();
    std::advance(i, index.row());
    switch(index.column())
    {
      case 0: return QString::fromStdString(i->first);
      case 1: {
        auto vv = i->second.value;
        if (auto* v = boost::get<double>(&vv))
          return QString::number(*v);
        else if (auto* v = boost::get<arma::mat>(&vv))
          return QString::fromStdString(insight::toStr(*v));
        else if (auto* v = boost::get<std::string>(&vv))
          return QString::fromStdString(*v);
        else
          return QVariant();
      }
      case 2: return QString::fromStdString(i->second.unit);
      case 3: return QString::fromStdString(i->second.description);
    }
  }

  return QVariant();
}

void IQSupplementedInputDataModel::reset(
    const insight::supplementedInputDataBase::ReportedSupplementQuantitiesTable &data
    )
{
  beginResetModel();
  reportedSupplementQuantities_=data;
  endResetModel();
}
