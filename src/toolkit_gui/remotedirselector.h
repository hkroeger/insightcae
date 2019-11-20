#ifndef REMOTEDIRSELECTOR_H
#define REMOTEDIRSELECTOR_H


#include <QDialog>
#include <QFileSystemModel>

#include "base/boost_include.h"
#include "openfoam/mountremote.h"

namespace Ui {
  class RemoteDirSelector;
}



class RemoteDirSelector : public QDialog
{
  Q_OBJECT

  bfs_path mountpoint_;

  std::shared_ptr<insight::MountRemote> mount_;

  QFileSystemModel *fs_model_;

public:
  explicit RemoteDirSelector(QWidget *parent = nullptr, const std::string& defaultServer = "");
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
