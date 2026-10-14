#include "iqremoteserverlistmodel.h"

#include "base/exception.h"
#include "base/remoteserver.h"
#include "base/tools.h"
#include "base/cppextensions.h"

#include "iqremoteserverstatus.h"
#include "base/translations.h"

#include <QFont>
#include <QIcon>
#include <qnamespace.h>

IQRemoteServerListModel::IQRemoteServerListModel(QObject *parent)
  : QAbstractItemModel(parent)
{
    for (auto &rs: insight::remoteServers())
    {
        if (!rs->wasExpanded())
        {
            auto crs=rs->clone();
            remoteServers_.push_back(crs);
            if (rs==insight::remoteServers().getPreferredServer())
                preferredServer_=crs;
        }
    }

    for (auto &rsp: insight::remoteServers().serverPools())
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
      case NameColumn: return _("Server Label");
      case StatusColumn: return _("Status");
    }
  }
  return QVariant();
}

QModelIndex IQRemoteServerListModel::index(int row, int column, const QModelIndex &parent) const
{
  if (!parent.isValid())
  {
    if (row>=0 && row<remoteServers_.size() && column>=0 && column<2)
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
  return 2;
}

QVariant IQRemoteServerListModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid())
    {
        auto i=remoteServers_.begin();
        std::advance(i, index.row());

        if (index.column()==StatusColumn)
        {
            auto rsc = boost::get<insight::RemoteServer::ConfigPtr>(&*i);
            if (!rsc) return QVariant(); // server pool: no state

            auto s = status_.find(rsc->get());
            using IQRemoteServerStatus::State;

            if (s==status_.end() || !s->second.online)
            {
                if (role==Qt::DecorationRole) return IQRemoteServerStatus::icon(State::Unknown);
                if (role==Qt::DisplayRole) return _("checking...");
            }
            else if (!*s->second.online)
            {
                if (role==Qt::DecorationRole) return IQRemoteServerStatus::icon(State::Offline);
                if (role==Qt::DisplayRole || role==Qt::ToolTipRole)
                {
                    if (const auto& ex = s->second.exists)
                        return *ex ? _("not reachable (instance exists)")
                                   : _("instance does not exist");
                    return _("not reachable");
                }
            }
            else if (const auto& l = s->second.load)
            {
                if (role==Qt::DecorationRole)
                    return IQRemoteServerStatus::isBusy(*l) ?
                        IQRemoteServerStatus::icon(State::Busy, l->utilization) :
                        IQRemoteServerStatus::icon(State::Online);
                if (role==Qt::DisplayRole || role==Qt::ToolTipRole)
                    return IQRemoteServerStatus::describe(*l);
            }
            else
            {
                if (role==Qt::DecorationRole) return IQRemoteServerStatus::icon(State::Online);
                if (role==Qt::DisplayRole || role==Qt::ToolTipRole) return _("online");
            }
            return QVariant();
        }

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

  checkStatus(index(inew, 0));
}

void IQRemoteServerListModel::removeRemoteServer(const QModelIndex &index)
{
    auto i=remoteServers_.begin();
    std::advance(i, index.row());
    if (auto rsc = boost::get<insight::RemoteServer::ConfigPtr>(&*i))
        status_.erase(rsc->get()); // pending results are discarded (row not found)
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    remoteServers_.erase(i);
    endRemoveRows();
}




int IQRemoteServerListModel::rowOf(const insight::RemoteServer::Config* cfg) const
{
    for (size_t r=0; r<remoteServers_.size(); ++r)
    {
        if (auto rsc = boost::get<insight::RemoteServer::ConfigPtr>(&remoteServers_[r]))
            if (rsc->get()==cfg) return int(r);
    }
    return -1;
}




void IQRemoteServerListModel::statusUpdated(const insight::RemoteServer::Config* cfg)
{
    int r=rowOf(cfg);
    if (r>=0)
    {
        auto i=index(r, StatusColumn);
        Q_EMIT dataChanged(i, i);
    }
}




void IQRemoteServerListModel::checkStatus(const QModelIndex& idx)
{
    if (!idx.isValid() || idx.row()>=int(remoteServers_.size())) return;

    auto rsc = boost::get<insight::RemoteServer::ConfigPtr>(&remoteServers_[idx.row()]);
    if (!rsc) return; // server pool

    insight::RemoteServer::ConfigPtr cfg = *rsc;
    const auto* key = cfg.get();

    auto& s = status_[key];
    s.online.reset();
    s.load.reset();
    s.exists.reset();
    int gen = s.generation = ++lastGeneration_;
    statusUpdated(key);

    auto current = [this,key,gen]() -> ServerStatus*
    {
        auto s = status_.find(key);
        if (s==status_.end() || s->second.generation!=gen) return nullptr;
        return &s->second;
    };

    IQRemoteServerStatus::checkInBackground(
        cfg, this,
        [this,key,current](bool online)
        {
            if (auto *s=current())
            {
                s->online=online;
                statusUpdated(key);
            }
        },
        [this,key,current](const insight::RemoteServer::Config::ProcessorLoad& l)
        {
            if (auto *s=current())
            {
                s->load=l;
                statusUpdated(key);
            }
        },
        [this,key,current](bool exists)
        {
            if (auto *s=current())
            {
                s->exists=exists;
                statusUpdated(key);
            }
        } );
}




void IQRemoteServerListModel::checkAllStatus()
{
    for (int r=0; r<rowCount(); ++r)
        checkStatus(index(r, 0));
}




boost::optional<bool> IQRemoteServerListModel::isOnline(const QModelIndex& idx) const
{
    if (!idx.isValid() || idx.row()>=int(remoteServers_.size())) return boost::none;
    auto rsc = boost::get<insight::RemoteServer::ConfigPtr>(&remoteServers_[idx.row()]);
    if (!rsc) return boost::none;
    auto s = status_.find(rsc->get());
    if (s==status_.end()) return boost::none;
    return s->second.online;
}




boost::optional<bool> IQRemoteServerListModel::exists(const QModelIndex& idx) const
{
    if (!idx.isValid() || idx.row()>=int(remoteServers_.size())) return boost::none;
    auto rsc = boost::get<insight::RemoteServer::ConfigPtr>(&remoteServers_[idx.row()]);
    if (!rsc) return boost::none;
    auto s = status_.find(rsc->get());
    if (s==status_.end()) return boost::none;
    return s->second.exists;
}




bool IQRemoteServerListModel::isServerPool(const QModelIndex& idx) const
{
    if (!idx.isValid() || idx.row()>=int(remoteServers_.size())) return false;
    return boost::get<insight::RemoteServerPoolConfig>(&remoteServers_[idx.row()]) != nullptr;
}
