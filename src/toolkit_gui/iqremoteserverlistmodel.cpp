#include "iqremoteserverlistmodel.h"

#include "base/tools.h"

IQRemoteServerListModel::IQRemoteServerListModel(QObject *parent)
  : QAbstractItemModel(parent),
    remoteServers_(insight::remoteServers)
{
}

QVariant IQRemoteServerListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation==Qt::Horizontal && role==Qt::DisplayRole)
  {
    switch (section)
    {
      case 0: return "Server Label";
    }
  }
  return QVariant();
}

QModelIndex IQRemoteServerListModel::index(int row, int column, const QModelIndex &parent) const
{
  if (!parent.isValid())
  {
    if (row>=0 && row<remoteServers_.size())
    {
      return createIndex(row, column, nullptr);
    }
  }
  return QModelIndex();
}

QModelIndex IQRemoteServerListModel::parent(const QModelIndex &) const
{
  return QModelIndex();
}

int IQRemoteServerListModel::rowCount(const QModelIndex &/*parent*/) const
{
  return remoteServers_.size();
}

int IQRemoteServerListModel::columnCount(const QModelIndex &/*parent*/) const
{
  return 1;
}

QVariant IQRemoteServerListModel::data(const QModelIndex &index, int role) const
{
  if (role==Qt::DisplayRole)
  {
    if (index.isValid())
    {
      auto i=remoteServers_.begin();
      std::advance(i, index.row());
      return QString::fromStdString( *(*i) );
    }
  }

  return QVariant();
}

insight::RemoteServerList &IQRemoteServerListModel::remoteServers()
{
  return remoteServers_;
}

insight::RemoteServerList::iterator IQRemoteServerListModel::getRemoteServer(const QModelIndex &index)
{
  auto i=remoteServers_.begin();
  std::advance(i, index.row());
  return i;
}

void IQRemoteServerListModel::addRemoteServer(insight::RemoteServer::ConfigPtr newRemoteServer)
{
  int inew=insight::predictSetInsertionLocation(remoteServers_, newRemoteServer);
  insight::dbg()<<"inew="<<inew<<std::endl;
  beginInsertRows(QModelIndex(), inew, inew);
  remoteServers_.insert(newRemoteServer);
  endInsertRows();
}

void IQRemoteServerListModel::removeRemoteServer(insight::RemoteServerList::iterator rsi)
{
  int i=std::distance(remoteServers_.begin(), rsi);
  beginRemoveRows(QModelIndex(), i, i);
  remoteServers_.erase(rsi);
  endRemoveRows();
}
