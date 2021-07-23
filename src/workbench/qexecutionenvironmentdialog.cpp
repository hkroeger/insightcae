#include "qexecutionenvironmentdialog.h"
#include "ui_qexecutionenvironmentdialog.h"

#include "base/wsllinuxserver.h"
#include "base/sshlinuxserver.h"

#include <QFileDialog>

void QExecutionEnvironmentDialog::resetServerName()
{
  if (remoteLocation_)
  {
    auto serverName =
        QString::fromStdString( *remoteLocation_->serverConfig() );
    if (!serverName.isEmpty())
    {
      ui->cbHost->setCurrentIndex(
          ui->cbHost->findText( serverName )
          );
    }
  }
}

void QExecutionEnvironmentDialog::resetRemoteWorkingDir()
{
  if (remoteLocation_)
  {
    ui->leRemoteDirectory->setText(
          QString::fromStdString(
            remoteLocation_->remoteDir().string() ) );
  }
}

void QExecutionEnvironmentDialog::setInvalidWorkingDir(bool changeLE)
{
  if (changeLE) ui->leLocalWorkingDirectory->setText("");
  ui->gbPerformRemoteExecution->setEnabled(false);
  remoteLocation_.reset();
}

bool QExecutionEnvironmentDialog::checkAndUpdateWorkingDir(const QString& newDir, bool changeLE)
{
  auto newDirPath = newDir.toStdString();

  if (newDirPath.empty())
  {
    setInvalidWorkingDir(changeLE);
    return false;
  }
  else
  {
    if (boost::filesystem::exists(newDirPath))
    {
      if (!remoteLocation_)
        setRemoteConfigFromWorkingDir();
      return true;
    }
    else
    {
      setInvalidWorkingDir(changeLE);
      return false;
    }
  }
}

bool QExecutionEnvironmentDialog::checkAndUpdateRemoteConfig(
    const QString& serverName,
    const QString& newDir )
{
  auto newDirPath = newDir.toStdString();
  std::unique_ptr<insight::RemoteLocation> rl(
        new insight::RemoteLocation(
          insight::remoteServers.findServer(
            serverName.toStdString() ),
          newDirPath,
          false
          )
        );

  if (auto wsl = std::dynamic_pointer_cast<insight::WSLLinuxServer>(rl->server()))
    insight::dbg()<<"WSL "<<wsl->serverConfig()->WSLExecutable_<<std::endl;
  if (auto ssh = std::dynamic_pointer_cast<insight::SSHLinuxServer>(rl->server()))
    insight::dbg()<<"SSH "<<ssh->serverConfig()->hostName_<<std::endl;

  if ( newDirPath.empty() || (!newDirPath.empty() && rl->remoteDirExists() ) )
  {
    //ok
    remoteLocation_ = std::move(rl);
    ui->leRemoteDirectory->setPalette(defaultPal_);
    ui->leRemoteDirectory->setToolTip("");
    return true;
  }
  else
  {
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::red);
    palette.setColor(QPalette::Text, Qt::black);
    ui->leRemoteDirectory->setPalette(palette);
    ui->leRemoteDirectory->setToolTip("The remote directory must exist. If you want an auto-created remote directory, leave the input empty!");
    return false;
  }
}


void QExecutionEnvironmentDialog::setRemoteConfigFromWorkingDir()
{
  auto lwd = localDirectory();
  if (!lwd.empty())
  {
    try
    {
      std::unique_ptr<insight::RemoteLocation> rl(
            new insight::RemoteLocation(lwd)
            );
      insight::dbg()<<"remote location configuration read from directory "<<lwd<<std::endl;

      remoteLocation_ = std::move(rl);

      resetServerName();
      resetRemoteWorkingDir();
    }
    catch (...)
    {
      insight::dbg()<<"no remote location configuration in directory "<<lwd<<std::endl;
    }
  }
}


QExecutionEnvironmentDialog::QExecutionEnvironmentDialog(
    const insight::CaseDirectory* localDirectory,
    const insight::RemoteLocation* remoteLocation,
    QWidget *parent )
: QDialog(parent),
  ui(new Ui::QExecutionEnvironmentDialog)
{
  ui->setupUi(this);
  defaultPal_ = ui->leRemoteDirectory->palette();

  for (const auto& i: insight::remoteServers)
  {
    ui->cbHost->addItem( QString::fromStdString(*i) );
  }

  connect(ui->leLocalWorkingDirectory, &QLineEdit::textChanged, this,
          [&](const QString& newDir)
          {
            if (checkAndUpdateWorkingDir(newDir, false))
              lastValidLocalWorkDirSetting_=newDir;
          }
  );
  connect(ui->leLocalWorkingDirectory, &QLineEdit::editingFinished, this,
          [&]()
          {
            QString newDir=ui->leLocalWorkingDirectory->text();
            if (checkAndUpdateWorkingDir(newDir, true))
              lastValidLocalWorkDirSetting_=newDir;
            else
              ui->leLocalWorkingDirectory->setText(lastValidLocalWorkDirSetting_);
          }
  );

  connect(ui->cbHost, &QComboBox::currentTextChanged, this,
          [&](const QString &srv)
          {
            checkAndUpdateRemoteConfig(
                  srv,
                  ui->leRemoteDirectory->text() );
          }
  );

  connect(ui->leRemoteDirectory, &QLineEdit::textChanged, this,
          [&](const QString& newDir)
          {
            checkAndUpdateRemoteConfig(
                  ui->cbHost->currentText(),
                  newDir );
          }
  );

  connect(ui->btnSelectLocalWorkingDirectory, &QPushButton::clicked, this,
         [&]()
         {
           QString dir = QFileDialog::getExistingDirectory(
                 this,
                 "Please select working directory",
                 ui->leLocalWorkingDirectory->text()
                 );
           if (!dir.isEmpty())
           {
             ui->leLocalWorkingDirectory->setText(dir); // invokes "textChanged" handler above
           }
         }
  );

  connect(this, &QDialog::accepted, this,
          [&]()
          {
            remoteLocation_.reset(
                  new insight::RemoteLocation(
                    insight::remoteServers.findServer(
                      ui->cbHost->currentText().toStdString() ),
                    ui->leRemoteDirectory->text().toStdString()
                    )
                  );
          }
  );


  if (localDirectory)
  {
    ui->leLocalWorkingDirectory->setText( QString::fromStdString(localDirectory->string()) ); // remoteLocation_ will be set by signal handler above
  }
  lastValidLocalWorkDirSetting_ = ui->leLocalWorkingDirectory->text();

  if (remoteLocation)
  {
    remoteLocation_.reset(
          new insight::RemoteLocation(*remoteLocation_));

    ui->gbPerformRemoteExecution->setChecked(true);
    resetServerName();
    resetRemoteWorkingDir();
  }

}


boost::filesystem::path QExecutionEnvironmentDialog::localDirectory() const
{
  return ui->leLocalWorkingDirectory->text().toStdString();
}

insight::RemoteLocation* QExecutionEnvironmentDialog::remoteLocation() const
{
  return remoteLocation_.get();
}


QExecutionEnvironmentDialog::~QExecutionEnvironmentDialog()
{
  delete ui;
}
