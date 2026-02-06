#include "remotedirselector.h"
#include "ui_remotedirselector.h"

#include "base/exception.h"
#include "base/remoteexecution.h"
#include "base/remoteserverlist.h"
#include "base/sshlinuxserver.h"
#include "base/translations.h"
#include <cstdlib>

#include <QDebug>
#include <QInputDialog>




RemoteDirSelector::RemoteDirSelector(QWidget *parent, insight::RemoteServerPtr server)
: QDialog(parent),
  server_(server),
  ui(new Ui::RemoteDirSelector)
{
  ui->setupUi(this);
  setWindowTitle(_("Select Remote Directory"));

  fs_model_ = new IQRemoteFolderModel(this, server, server->config().defaultDirectory_);
  ui->directory->setModel(fs_model_);

  connect(ui->createDirTopLevel, &QPushButton::clicked, this,
          std::bind(&RemoteDirSelector::createDirBelow, this, QModelIndex()) );
  connect(ui->createDirBelowSelected, &QPushButton::clicked, this,
          std::bind(&RemoteDirSelector::createDirBelow, this,
            std::bind(&QTreeView::currentIndex, ui->directory) ) );


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




 void RemoteDirSelector::createDirBelow(QModelIndex i)
 {
     boost::filesystem::path baseDir(fs_model_->folder(i));

     bool ok=false;
     bfs_path newdir
         (
             QInputDialog::getText(
                 this,
                 "Create directory",
                 "Create directory below "+QString::fromStdString(baseDir.string()),
                 QLineEdit::Normal, "",
                 &ok
                 ).toStdString()
             );
     if (ok)
     {
         server_->createDirectory(baseDir/newdir);
         fs_model_->refreshBelow(i);
     }
 }

