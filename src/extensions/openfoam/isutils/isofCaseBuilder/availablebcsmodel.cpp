#include "availablebcsmodel.h"

#include "openfoam/caseelements/boundarycondition.h"

AvailableBCsModel::AvailableBCsModel(QObject *parent)
  : QAbstractListModel(parent)
{
  for (
       auto i = insight::BoundaryCondition::factories_->begin();
        i != insight::BoundaryCondition::factories_->end();
       ++i )
  {
      BCtypes_.append(i->first);
  }
}

QVariant AvailableBCsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation==Qt::Horizontal && role==Qt::DisplayRole)
  {
    switch (section)
    {
      case 0: return "BC type";
    }
  }
  return QVariant();
}

int AvailableBCsModel::rowCount(const QModelIndex &parent) const
{
  // For list models only the root node (an invalid parent) should return the list's size. For all
  // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
  if (parent.isValid())
    return 0;

  return BCtypes_.size();
}


int AvailableBCsModel::columnCount(const QModelIndex &parent) const
{
  return 1;
}



QVariant AvailableBCsModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (role==Qt::DisplayRole)
  {
    switch (index.column())
    {
      case 0:
        return QString::fromStdString( BCtypes_[index.row()] );
    }
  }

  return QVariant();
}

std::string AvailableBCsModel::selectedBCType(const QModelIndex &index) const
{
  if (index.isValid())
  {
    return BCtypes_[index.row()];
  }
  return std::string();
}
