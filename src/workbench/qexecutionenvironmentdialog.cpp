#include "qexecutionenvironmentdialog.h"
#include "ui_qexecutionenvironmentdialog.h"

#include "base/wsllinuxserver.h"
#include "base/sshlinuxserver.h"

#include <QFileDialog>
#include "remotedirselector.h"



void QExecutionEnvironmentDialog::resetServerName()
{
  if (remoteLocation_)
  {
    auto serverName =
        QString::fromStdString( remoteLocation_->serverLabel() );
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
    if (newDir.isEmpty())
    {
      return true;
    }
    else if (boost::filesystem::exists(newDirPath))
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
  remoteLocation_.reset(
        new insight::RemoteLocation(
          insight::remoteServers.findServer(
            serverName.toStdString() ),
          newDirPath,
          false
          ) );

  return true;
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

  // populate controls

  defaultPal_ = ui->leRemoteDirectory->palette();

  for (const auto& i: insight::remoteServers)
  {
    ui->cbHost->addItem( QString::fromStdString(*i) );
  }

  // connect actions

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

  connect(ui->btnSelectRemoteDirectory, &QPushButton::clicked, this,
          [&]()
          {
            insight::dbg()<<"1"<<std::endl;
            if (remoteLocation_)
            {
              insight::dbg()<<"2"<<std::endl;
              if (auto server = remoteLocation_->serverConfig()->getInstanceIfRunning())
              {
                insight::dbg()<<"3"<<std::endl;
                RemoteDirSelector dlg(this, server);
                if (dlg.exec()==QDialog::Accepted)
                {
                  auto dir=dlg.selectedRemoteDir().generic_path();
                  insight::dbg()<<dir<<std::endl;
                  checkAndUpdateRemoteConfig(
                        ui->cbHost->currentText(),
                        QString::fromStdString(dir.string()) );
                  resetRemoteWorkingDir();
                }
              }
            }
          }
  );

  connect(this, &QDialog::accepted, this,
          [&]()
          {
            if (ui->gbPerformRemoteExecution->isChecked())
            {
              remoteLocation_.reset(
                    new insight::RemoteLocation(
                      insight::remoteServers.findServer(
                        ui->cbHost->currentText().toStdString() ),
                      ui->leRemoteDirectory->text().toStdString()
                      )
                    );
            }
            else
            {
              remoteLocation_.reset();
            }
          }
  );


  // set initial values
  if (localDirectory)
  {
    ui->leLocalWorkingDirectory->setText( QString::fromStdString(localDirectory->string()) ); // remoteLocation_ will be set by signal handler above
  }
  lastValidLocalWorkDirSetting_ = ui->leLocalWorkingDirectory->text();

  if (remoteLocation)
  {
    remoteLocation_.reset(
          new insight::RemoteLocation(*remoteLocation));

    ui->gbPerformRemoteExecution->setChecked(true);

    auto serverName =
        QString::fromStdString( remoteLocation_->serverLabel() );

    if (!serverName.isEmpty())
    {
      ui->cbHost->setCurrentIndex(
          ui->cbHost->findText( serverName )
          );
    }

    ui->leRemoteDirectory->setText(
          QString::fromStdString(
            remoteLocation_->remoteDir().string() ) );
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
