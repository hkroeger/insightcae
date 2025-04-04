#include "remotedirselector.h"
#include "ui_remotedirselector.h"

#include "base/exception.h"
#include "base/remoteexecution.h"
#include "base/remoteserverlist.h"
#include "base/sshlinuxserver.h"
#include <cstdlib>

#include <QDebug>
#include <QInputDialog>




RemoteDirSelector::RemoteDirSelector(QWidget *parent, insight::RemoteServerPtr server)
: QDialog(parent),
  server_(server),
  ui(new Ui::RemoteDirSelector)
{
  ui->setupUi(this);

  fs_model_ = new IQRemoteFolderModel(this, server, server->config().defaultDirectory_);
  ui->directory->setModel(fs_model_);

  connect(ui->newdir, &QPushButton::clicked,
          this, &RemoteDirSelector::createDir);


  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &RemoteDirSelector::accept);
}

RemoteDirSelector::~RemoteDirSelector()
{
    delete ui;
}



bfs_path RemoteDirSelector::selectedRemoteDir()
{
    return fs_model_->folder( ui->directory->currentIndex() );
}



 void RemoteDirSelector::createDir()
 {
     QModelIndex i=ui->directory->currentIndex();
     if (i.isValid())
     {
         bool ok;
         bfs_path base_dir( fs_model_->folder(i) );

         bfs_path newdir
         (
           QInputDialog::getText(
                  this,
                  "Create directory",
                  "Create directory below "+QString::fromStdString(base_dir.string()),
                  QLineEdit::Normal, "",
                  &ok
                 ).toStdString()
         );
         if (ok)
         {
             server_->createDirectory(base_dir/newdir);
             fs_model_->refreshBelow(i);
         }
     }
 }

