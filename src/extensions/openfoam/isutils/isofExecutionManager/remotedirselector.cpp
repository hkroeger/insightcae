#include "remotedirselector.h"
#include "ui_remotedirselector.h"

#include "base/exception.h"
#include "openfoam/remoteexecution.h"
#include <cstdlib>

#include <QDebug>
#include <QInputDialog>


bool MountRemote::isAlreadyMounted() const
{
  std::cout<<"reading /proc/mounts"<<std::endl;
  std::ifstream f("/proc/mounts");
  std::string line;
  boost::regex e ("^([^ ]+) ([^ ]+) .*$");
  while (getline(f, line))
  {
    boost::smatch m;
    if (boost::regex_search(line, m, e))
    {
      std::string src=m[1], mp=m[2];
      std::cout<<src<<" "<<mp<<std::endl;
      if ( boost::filesystem::equivalent(mp, mountpoint_) )
      {
        std::cout<<mp<< " >> matches "<<mountpoint_<<std::endl;
        return true;
      }
    }
  }
  return false;
}


void MountRemote::mount(const std::string& server, const bfs_path& remotedir)
{
  auto gid = getgid();
  auto uid = getuid();

  std::string cmd=boost::str(boost::format( "sshfs -o uid=%d,gid=%d,follow_symlinks \"%s:%s\" \"%s\"") % uid % gid % server % remotedir.string() % mountpoint_.string() );
  std::cout<<cmd<<std::endl;
  if (std::system( cmd.c_str() )!=0)
      throw insight::Exception("Could not mount remote filesystem. Failed command was: "+cmd);
}


void MountRemote::unmount()
{
  std::string cmd="fusermount -z -u \""+mountpoint_.string()+"\"";
  std::system(cmd.c_str());
}

MountRemote::MountRemote(const bfs_path& mountpoint, const std::string& server, const bfs_path& remotedir, bool keep, bool expect_mounted)
    : mountpoint_(mountpoint), keep_(keep)
{
  bool is_mounted=isAlreadyMounted();

  if (is_mounted && !expect_mounted)
    throw insight::Exception("Trying to mount to directory, which is already mounted!");
  else if (!is_mounted && expect_mounted)
    throw insight::Exception("Expected mounted directory, but found it unmounted!");

  if (!expect_mounted)
    mount(server, remotedir);
}


MountRemote::~MountRemote()
{
  if (!keep_)
  {
    unmount();
  }
}




RemoteDirSelector::RemoteDirSelector(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::RemoteDirSelector)
{
  ui->setupUi(this);

  for (const auto& s: insight::remoteServers)
    ui->server->addItem( /*s.serverName_.c_str()*/ s.first.c_str() );

  mountpoint_ = boost::filesystem::unique_path( boost::filesystem::temp_directory_path()/"remote-%%%%-%%%%-%%%%-%%%%" );
  boost::filesystem::create_directories(mountpoint_);

  fs_model_=new QFileSystemModel(this);
  ui->directory->setModel(fs_model_);

  connect(ui->server, &QComboBox::currentTextChanged,
          this, &RemoteDirSelector::serverChanged);

  connect(ui->newdir, &QPushButton::clicked,
          this, &RemoteDirSelector::createDir);

  ui->server->setCurrentIndex(0);

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
    return insight::remoteServers[ui->server->currentText().toStdString()].serverName_;
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
    mount_.reset(new MountRemote(mountpoint_, insight::remoteServers[name.toStdString()].serverName_, "/"));

    QString mp=QString::fromStdString(boost::filesystem::absolute(mountpoint_).string());
    qDebug()<<mp;
    fs_model_->setRootPath(mp);

    QModelIndex idx = fs_model_->index(mp);
    qDebug()<<idx;
    ui->directory->setRootIndex(idx);

//    auto i=std::find_if(insight::remoteServers.begin(),
//                        insight::remoteServers.end(),
//                        [&](const insight::RemoteServerInfo& s)
//                         { return s.serverName_==name.toStdString(); }
//    );
    auto i = insight::remoteServers.find(name.toStdString());
    if (i!=insight::remoteServers.end())
    {
        auto ci = fs_model_->index( (mountpoint_/i->second.defaultDir_).c_str() );
        ui->directory->setCurrentIndex(ci);
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
                   QInputDialog::getText(this,
                                          "Create directory",
                                          "Create directory below "+QString::fromStdString(dir.string()),
                                          QLineEdit::Normal, "",
                                          &ok).toStdString()
                 );
         if (ok)
         {
             boost::filesystem::create_directory(base_dir/newdir);
             auto ci = fs_model_->index( (base_dir/newdir).c_str() );
             ui->directory->setCurrentIndex(ci);
         }
     }
 }
