#include "remotedirselector.h"
#include "ui_remotedirselector.h"

#include "base/exception.h"
#include "base/remoteexecution.h"
#include "base/remoteserverlist.h"
#include "base/sshlinuxserver.h"
#include <cstdlib>

#include <QDebug>
#include <QInputDialog>







RemoteDirSelector::RemoteDirSelector(QWidget *parent, const std::string& defaultServer) :
  QDialog(parent),
  ui(new Ui::RemoteDirSelector)
{
  ui->setupUi(this);

  for (const auto& s: insight::remoteServers)
    ui->server->addItem( QString::fromStdString(*s) );

  mountpoint_ = boost::filesystem::unique_path( boost::filesystem::temp_directory_path()/"remote-%%%%-%%%%-%%%%-%%%%" );
  boost::filesystem::create_directories(mountpoint_);

  fs_model_=new QFileSystemModel(this);
  ui->directory->setModel(fs_model_);

  connect(ui->server, &QComboBox::currentTextChanged,
          this, &RemoteDirSelector::serverChanged);

  connect(ui->newdir, &QPushButton::clicked,
          this, &RemoteDirSelector::createDir);

  int i=0;
  if (!defaultServer.empty())
  {
    i=ui->server->findText(QString::fromStdString(defaultServer));
  }
  ui->server->setCurrentIndex(i);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &RemoteDirSelector::accept);
}

RemoteDirSelector::~RemoteDirSelector()
{
    mount_.reset();
    boost::filesystem::remove(mountpoint_);
    delete ui;
}


std::string RemoteDirSelector::selectedServer()
{
  auto s = insight::remoteServers.findServer( ui->server->currentText().toStdString() );

  if ( s->isDynamicallyAllocated() )
    throw insight::Exception("Cannot select directories on dynamically allocated hosts");

  return *s;
}


bfs_path RemoteDirSelector::selectedRemoteDir()
{
    QModelIndex i = ui->directory->currentIndex();
    bfs_path base_dir( fs_model_->filePath(i).toStdString() );
    return "/" / boost::filesystem::make_relative(mountpoint_, base_dir);
}


void RemoteDirSelector::serverChanged(const QString& name)
{
  mount_.reset();

  auto s = insight::remoteServers.findServer( name.toStdString() );

  if (s->isDynamicallyAllocated())
  {
    throw insight::Exception("Cannot select directories on dynamically allocated hosts");
  }
  else
  {
    if (auto sshh = std::dynamic_pointer_cast<insight::SSHLinuxServer>(s->getInstanceIfRunning()))
    {
      mount_.reset(new insight::MountRemote(mountpoint_, sshh->hostName(), "/"));

      QString mp=QString::fromStdString(boost::filesystem::absolute(mountpoint_).string());
      qDebug()<<mp;
      fs_model_->setRootPath(mp);

      QModelIndex idx = fs_model_->index(mp);
      qDebug()<<idx;
      ui->directory->setRootIndex(idx);

      auto i = insight::remoteServers.findServer(name.toStdString());

      auto ci = fs_model_->index( QString::fromStdString((mountpoint_/i->defaultDirectory_).string()) );
      ui->directory->setCurrentIndex(ci);
    }
    else
      throw insight::Exception("Cannot select directories on non-linux hosts");
  }
}


 void RemoteDirSelector::createDir()
 {
     QModelIndex i=ui->directory->currentIndex();
     if (i.isValid())
     {
         bool ok;
         bfs_path base_dir( fs_model_->filePath(i).toStdString() );
         bfs_path dir = boost::filesystem::make_relative(mountpoint_, base_dir);

         bfs_path newdir
         (
           QInputDialog::getText(
                  this,
                  "Create directory",
                  "Create directory below "+QString::fromStdString(dir.string()),
                  QLineEdit::Normal, "",
                  &ok
                 ).toStdString()
         );
         if (ok)
         {
             boost::filesystem::create_directory(base_dir/newdir);
             auto ci = fs_model_->index( QString::fromStdString((base_dir/newdir).string()) );
             ui->directory->setCurrentIndex(ci);
         }
     }
 }

