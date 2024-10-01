#include "iqremoteserverlistmodel.h"

#include "base/exception.h"
#include "base/remoteserver.h"
#include "base/tools.h"
#include "base/cppextensions.h"

#include <QFont>
#include <QIcon>
#include <qnamespace.h>

IQRemoteServerListModel::IQRemoteServerListModel(QObject *parent)
  : QAbstractItemModel(parent)
{
    for (auto &rs: insight::remoteServers)
    {
        if (!rs->wasExpanded())
        {
            auto crs=rs->clone();
            remoteServers_.push_back(crs);
            if (rs==insight::remoteServers.getPreferredServer())
                preferredServer_=crs;
        }
    }

    for (auto &rsp: insight::remoteServers.serverPools())
    {
        remoteServers_.push_back(rsp);
    }
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
    if (index.isValid())
    {
        auto i=remoteServers_.begin();
        std::advance(i, index.row());

        if (role==Qt::DisplayRole)
        {
            if (auto rsc = boost::get<insight::RemoteServer::ConfigPtr>(&*i))
            {
                return QString::fromStdString( **rsc );
            }
            else if (auto rspc = boost::get<insight::RemoteServerPoolConfig>(&*i))
            {
                return QString::fromStdString( *rspc->configTemplate_ );
            }

        }
        else if (role==Qt::FontRole)
        {
            QFont f;
            if (auto rsc = boost::get<insight::RemoteServer::ConfigPtr>(&*i))
            {
                if (!preferredServer_.expired())
                {
                    if (*rsc == preferredServer_.lock())
                        f.setBold(true);
                }
            }
            return f;
        }
        else if (role==Qt::DecorationRole)
        {
            if (boost::get<insight::RemoteServer::ConfigPtr>(&*i))
            {
                return QIcon(":/icons/icon_remoteserver.svg");
            }
            else if (boost::get<insight::RemoteServerPoolConfig>(&*i))
            {
                return QIcon(":/icons/icon_remoteserverpool.svg");
            }
        }
    }

    return QVariant();
}




QModelIndex IQRemoteServerListModel::preferredServer() const
{
    if (!preferredServer_.expired())
    {
        auto prs = preferredServer_.lock();

        for ( auto i=remoteServers_.begin();
              i!=remoteServers_.end();
              ++i)
        {
            if (auto rs = boost::get<insight::RemoteServer::ConfigPtr>(&*i))
            {
                if (*rs==prs)
                    return index(std::distance(remoteServers_.begin(), i), 0);
            }
        }
    }

    return QModelIndex();
}

void IQRemoteServerListModel::setPreferredServer(const QModelIndex& index)
{
    auto ch1=preferredServer();
    auto ns=getRemoteServer(index);
    preferredServer_=ns;
    auto ch2=preferredServer();

    Q_EMIT dataChanged(ch1, ch1);
    Q_EMIT dataChanged(ch2, ch2);
}

const insight::RemoteServerList &IQRemoteServerListModel::remoteServers() const
{

  // split list
  std::set<insight::RemoteServer::ConfigPtr> servers;
  std::set<insight::RemoteServerPoolConfig> pools;
  std::string preferredServerLabel;

  for (auto& i: remoteServers_)
  {
      if (auto rsc = boost::get<insight::RemoteServer::ConfigPtr>(&i))
      {
          servers.insert(*rsc);
      }
      else if (auto rspc = boost::get<insight::RemoteServerPoolConfig>(&i))
      {
          pools.insert(*rspc);
      }
  }

  if (auto ps = preferredServer_.lock())
  {
      preferredServerLabel=*ps;
  }

  newRemoteServerList_=
      std::make_unique<insight::RemoteServerList>(
       servers, pools, preferredServerLabel
      );

  return *newRemoteServerList_;
}

insight::RemoteServer::ConfigPtr IQRemoteServerListModel::getRemoteServer(const QModelIndex &index)
{
  auto i=remoteServers_.begin();
  std::advance(i, index.row());
  if (auto rsc = boost::get<insight::RemoteServer::ConfigPtr>(&*i))
  {
      return *rsc;
  }
  else if (auto rspc = boost::get<insight::RemoteServerPoolConfig>(&*i))
  {
      return rspc->configTemplate_;
  }
  else
      throw insight::UnhandledSelection();

  return nullptr;
}

void IQRemoteServerListModel::addRemoteServer(insight::RemoteServer::ConfigPtr newRemoteServer)
{
  int inew=remoteServers_.size();

  beginInsertRows(QModelIndex(), inew, inew);
  remoteServers_.push_back(newRemoteServer);
  endInsertRows();
}

void IQRemoteServerListModel::removeRemoteServer(const QModelIndex &index)
{
    auto i=remoteServers_.begin();
    std::advance(i, index.row());
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    remoteServers_.erase(i);
    endRemoveRows();
}
