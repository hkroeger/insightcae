#ifndef IQREMOTESERVERLISTMODEL_H
#define IQREMOTESERVERLISTMODEL_H

#include "toolkit_gui_export.h"

#include "base/remoteserverlist.h"
#include "boost/variant.hpp"
#include "boost/optional.hpp"

#include <map>

#include <QAbstractItemModel>
#include <memory>

class TOOLKIT_GUI_EXPORT IQRemoteServerListModel
    : public QAbstractItemModel
{
  Q_OBJECT

  std::vector<
      boost::variant<
          insight::RemoteServer::ConfigPtr,
          insight::RemoteServerPoolConfig
          >
      > remoteServers_;

  std::weak_ptr<insight::RemoteServer::Config> preferredServer_;

  mutable std::unique_ptr<insight::RemoteServerList> newRemoteServerList_;

  /**
   * state of the servers (not of server pools), determined in background
   */
  struct ServerStatus
  {
    boost::optional<bool> online; // unset: being checked
    boost::optional<insight::RemoteServer::Config::ProcessorLoad> load;
    boost::optional<bool> exists; // only for unreachable servers, whose instance can be created
    int generation = 0; // to discard results of outdated checks
  };
  std::map<const insight::RemoteServer::Config*, ServerStatus> status_;
  int lastGeneration_ = 0; // model-wide: entries may be removed and their addresses reused

  int rowOf(const insight::RemoteServer::Config* cfg) const;
  void statusUpdated(const insight::RemoteServer::Config* cfg);

public:
  explicit IQRemoteServerListModel(QObject *parent = nullptr);

  // Header:
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  // Basic functionality:
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &index) const override;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  QModelIndex preferredServer() const;
  void setPreferredServer(const QModelIndex& index);

  const insight::RemoteServerList& remoteServers() const;

  insight::RemoteServer::ConfigPtr getRemoteServer(const QModelIndex &index);

  void addRemoteServer(insight::RemoteServer::ConfigPtr newRemoteServer);
  void removeRemoteServer(const QModelIndex &index);

  enum Column { NameColumn = 0, StatusColumn = 1 };

  /**
   * @brief checkStatus
   * determine reachability and load of the server at the index in background
   * (nothing for server pools)
   */
  void checkStatus(const QModelIndex& index);
  void checkAllStatus();

  /**
   * @brief isOnline
   * @return
   * unset, if the server is being checked or the entry is a server pool
   */
  boost::optional<bool> isOnline(const QModelIndex& index) const;

  /**
   * @brief exists
   * @return
   * unset, if unknown (being checked, reachable server, no existence check possible)
   */
  boost::optional<bool> exists(const QModelIndex& index) const;
  bool isServerPool(const QModelIndex& index) const;

private:
};

#endif // IQREMOTESERVERLISTMODEL_H
