#ifndef IQREMOTESERVERLISTMODEL_H
#define IQREMOTESERVERLISTMODEL_H

#include "toolkit_gui_export.h"

#include "base/remoteserverlist.h"

#include <QAbstractItemModel>

class TOOLKIT_GUI_EXPORT IQRemoteServerListModel
    : public QAbstractItemModel
{
  Q_OBJECT

  insight::RemoteServerList remoteServers_;

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

  insight::RemoteServerList& remoteServers();

  insight::RemoteServerList::iterator getRemoteServer(const QModelIndex &index);

  void addRemoteServer(insight::RemoteServer::ConfigPtr newRemoteServer);
  void removeRemoteServer(insight::RemoteServerList::iterator rsi);

private:
};

#endif // IQREMOTESERVERLISTMODEL_H
