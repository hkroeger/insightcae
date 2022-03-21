#ifndef REMOTEDIRSELECTOR_H
#define REMOTEDIRSELECTOR_H

#include "toolkit_gui_export.h"

#include <QDialog>

#include "base/boost_include.h"
#include "iqremotefoldermodel.h"

namespace Ui {
  class RemoteDirSelector;
}



class TOOLKIT_GUI_EXPORT RemoteDirSelector : public QDialog
{
  Q_OBJECT

  insight::RemoteServerPtr server_;
  IQRemoteFolderModel* fs_model_;

public:
  explicit RemoteDirSelector(QWidget *parent, insight::RemoteServerPtr server);
  ~RemoteDirSelector();

  bfs_path selectedRemoteDir();

private Q_SLOTS:
  void createDir();

private:
  Ui::RemoteDirSelector *ui;
};


#endif // REMOTEDIRSELECTOR_H
