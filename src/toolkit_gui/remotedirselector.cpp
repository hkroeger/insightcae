#include "remotedirselector.h"
#include "ui_remotedirselector.h"

#include "base/exception.h"
#include "base/remoteexecution.h"
#include "base/remoteserverlist.h"
#include "base/sshlinuxserver.h"
#include "base/translations.h"
#include <cstdlib>

#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QProgressBar>




RemoteDirSelector::RemoteDirSelector(QWidget *parent, insight::RemoteServerPtr server)
: QDialog(parent),
  server_(server),
  ui(new Ui::RemoteDirSelector)
{
  ui->setupUi(this);
  setWindowTitle(_("Select Remote Directory"));

  fs_model_ = new IQRemoteFolderModel(this, server, server->config().defaultDirectory_);
  ui->directory->setModel(fs_model_);

  // status line below the tree: shown, while remote folders are listed
  status_ = new QWidget;
  auto *sl = new QHBoxLayout(status_);
  sl->setContentsMargins(0,0,0,0);
  auto *busy = new QProgressBar;
  busy->setRange(0, 0); // indeterminate
  busy->setTextVisible(false);
  busy->setMaximumWidth(80);
  busy->setMaximumHeight(12);
  sl->addWidget(busy);
  statusLabel_ = new QLabel;
  statusLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  sl->addWidget(statusLabel_, 1);
  ui->verticalLayout->addWidget(status_);
  status_->hide();

  auto serverLabel = QString::fromStdString(server->config());
  connect(fs_model_, &IQRemoteFolderModel::listingStateChanged, this,
          [this,serverLabel](int pending, const QString& path)
          {
            status_->setVisible(pending>0);
            if (pending>0)
            {
              statusLabel_->setText(
                  QString(_("Listing %1 on %2...")).arg(path, serverLabel) );
            }
          } );

  // list the base folder now (in background), the dialog appears immediately
  fs_model_->fetchMore(QModelIndex());

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
         {
             QApplication::setOverrideCursor(Qt::WaitCursor);
             try
             {
                 server_->createDirectory(baseDir/newdir);
             }
             catch (...)
             {
                 QApplication::restoreOverrideCursor();
                 throw;
             }
             QApplication::restoreOverrideCursor();
         }
         fs_model_->refreshBelow(i); // in background
     }
 }

