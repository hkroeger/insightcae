#ifndef IQREMOTESERVERLISTMODEL_H
#define IQREMOTESERVERLISTMODEL_H

#include "toolkit_gui_export.h"

#include "base/remoteserverlist.h"

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

private:
};

#endif // IQREMOTESERVERLISTMODEL_H
