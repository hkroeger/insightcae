#ifndef REMOTEDIRSELECTOR_H
#define REMOTEDIRSELECTOR_H

#include <QDialog>
#include <QFileSystemModel>

#include "base/boost_include.h"

namespace Ui {
  class RemoteDirSelector;
}


struct ServerInfo
{
    std::string serverName_;
    bfs_path defaultDir_;

    ServerInfo(
                std::string serverName,
                bfs_path defaultDir
            );
};

extern std::vector<ServerInfo> servers;


class MountRemote
{
    bfs_path mountpoint_;

public:
    MountRemote(const bfs_path& mountpoint, const std::string& server, const bfs_path& remotedir);
    ~MountRemote();
};

class RemoteDirSelector : public QDialog
{
  Q_OBJECT

  bfs_path mountpoint_;

  std::shared_ptr<MountRemote> mount_;

  QFileSystemModel *fs_model_;

public:
  explicit RemoteDirSelector(QWidget *parent = nullptr);
  ~RemoteDirSelector();

  std::string selectedServer();
  bfs_path selectedRemoteDir();

private Q_SLOTS:
  void serverChanged(const QString& name);
  void createDir();

private:
  Ui::RemoteDirSelector *ui;
};

#endif // REMOTEDIRSELECTOR_H
