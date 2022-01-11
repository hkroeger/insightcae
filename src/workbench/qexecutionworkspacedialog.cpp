#include "qexecutionworkspacedialog.h"
#include "ui_qexecutionworkspacedialog.h"

#include "base/wsllinuxserver.h"
#include "base/sshlinuxserver.h"
#include "base/cppextensions.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QToolTip>
#include "remotedirselector.h"




bool QExecutionWorkspaceDialog::isTemporaryLocalDirectorySelected() const
{
    return ui->leLocalWorkingDirectory->text().isEmpty();
}




bool QExecutionWorkspaceDialog::isValidWorkingDir(const boost::filesystem::path& newDirPath) const
{
    return
            newDirPath.empty()
            ||
            (
               boost::filesystem::exists(newDirPath)
                &&
               boost::filesystem::is_directory(newDirPath)
            )
            ;
}



void QExecutionWorkspaceDialog::displayCurrentServerName()
{
    if (remoteLocation_)
    {
        auto serverName =
                QString::fromStdString( remoteLocation_->serverLabel() );
        if (!serverName.isEmpty())
        {
            ui->cbHost->blockSignals(true);
            ui->cbHost->setCurrentIndex(
                        ui->cbHost->findText( serverName )
                        );
            ui->cbHost->blockSignals(false);
        }
    }
}




void QExecutionWorkspaceDialog::displayCurrentRemoteWorkingDir()
{
    if (remoteLocation_)
    {
        ui->leRemoteDirectory->blockSignals(true);
        ui->leRemoteDirectory->setText(
                    QString::fromStdString(
                        remoteLocation_->remoteDir().string() ) );
        ui->leRemoteDirectory->blockSignals(false);
    }
}




void QExecutionWorkspaceDialog::changeWorkingDirectory(const QString& newDir)
{
  boost::filesystem::path newDirPath = newDir.toStdString();

  insight::dbg()<<"set new work dir:" <<newDir.toStdString()<<std::endl;

  if (!isValidWorkingDir(newDirPath))
  {
      QMessageBox::critical(this, "Invalid Working Directory",
                  "The selected working directory \""+newDir+"\" is invalid.\n"
                  "Please enter a valid working directory!");

      ui->leLocalWorkingDirectory->setText(lastValidLocalWorkDirSetting_);
      return;
  }

//  if (newDir.isEmpty() && ui->gbPerformRemoteExecution->isChecked())
//  {
//      ui->gbPerformRemoteExecution->setChecked(false);
//  }

  if (boost::filesystem::exists(newDirPath))
  {
      if (!remoteLocation_)
          setRemoteConfigFromWorkingDir();
  }

  lastValidLocalWorkDirSetting_=newDir;
}




void QExecutionWorkspaceDialog::checkAndChangeRemoteConfig(
        const QString& serverName,
        const QString& newDir )
{
    auto newDirPath = newDir.toStdString();

    auto rl = std::make_unique<insight::RemoteLocation>(
                insight::remoteServers.findServer(
                    serverName.toStdString() ),
                newDirPath,
                newDirPath.empty()?true:false
                                   );

    if (!newDirPath.empty())
    {
        if (rl->serverIsAvailable())
        {
            if (!rl->isActive())
            {
                ui->leRemoteDirectory->setStyleSheet("QLineEdit{background: red;}");
                ui->leRemoteDirectory->setToolTip("The remote direcotry is invalid!");
                return;
            }
            else
            {
                ui->leRemoteDirectory->setStyleSheet("QLineEdit{background: lightgreen;}");
                ui->leRemoteDirectory->setToolTip("Verified.");
            }
        }
        else
        {
            ui->leRemoteDirectory->setStyleSheet("QLineEdit{background: yellow;}");
            ui->leRemoteDirectory->setToolTip("The remote directory cannot be checked because the server is unreachable yet!\n"
                                              "Please make sure that it is existing!");
        }
    }
    else
    {
        ui->leRemoteDirectory->setStyleSheet("");
        ui->leRemoteDirectory->setToolTip("");
    }

    remoteLocation_ = std::move(rl);

}




void QExecutionWorkspaceDialog::setRemoteConfigFromWorkingDir()
{
  auto lwd = localDirectory();
  if (!lwd.empty())
  {
    try
    {
      auto rl = std::make_unique<insight::RemoteLocation>(
                  insight::RemoteExecutionConfig::defaultConfigFile(lwd) );

      insight::dbg()<<"remote location configuration read from directory "<<lwd<<std::endl;

      remoteLocation_ = std::move(rl);

      ui->gbPerformRemoteExecution->setChecked(true);
      displayCurrentServerName();
      displayCurrentRemoteWorkingDir();
    }
    catch (...)
    {
      insight::dbg()<<"no remote location configuration in directory "<<lwd<<std::endl;
    }
  }
}




QExecutionWorkspaceDialog::QExecutionWorkspaceDialog(
    const insight::CaseDirectory* localDirectory,
    const insight::RemoteLocation* remoteLocation,
    QWidget *parent )
: QDialog(parent),
  ui(new Ui::QExecutionWorkspaceDialog),
  lockRemoteExecution_(false)
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
            insight::dbg()<<"WD changed: "<<newDir.toStdString()<<std::endl;
            if (isValidWorkingDir(newDir.toStdString()))
              lastValidLocalWorkDirSetting_=newDir;
          }
  );
  connect(ui->leLocalWorkingDirectory, &QLineEdit::editingFinished, this,
          [&]()
          {
            QString newDir=ui->leLocalWorkingDirectory->text();
            insight::dbg()<<"WD edit finished: "<<newDir.toStdString()<<std::endl;
            changeWorkingDirectory(newDir);
          }
  );

  connect(ui->gbPerformRemoteExecution, &QGroupBox::toggled, this,
          [&](bool on)
          {
            if (!on && lockRemoteExecution_)
            {
                ui->gbPerformRemoteExecution->setChecked(true);
                return;
            }

            if (on && isTemporaryLocalDirectorySelected())
            {
//                auto answer = QMessageBox::question(
//                            this, "Invalid working directory",
//                            "When performing remote execution, "
//                            "the local working directory must not be temporary!\n"
//                            "Please select an existing working directory first!",
//                            QMessageBox::Ok );
//                ui->gbPerformRemoteExecution->setChecked(false);

                QString msg = "Please note: the local directory will be auto-created but not removed!";
                ui->leLocalWorkingDirectory->setToolTip(msg);
                QToolTip::showText(
                            ui->leLocalWorkingDirectory->mapToGlobal(ui->leLocalWorkingDirectory->pos()),
                            msg);
            }
          }
  );

  connect(ui->cbHost, &QComboBox::currentTextChanged, this,
          [&](const QString &srv)
          {
            checkAndChangeRemoteConfig(
                  srv,
                  ui->leRemoteDirectory->text() );
          }
  );

  connect(ui->leRemoteDirectory, &QLineEdit::editingFinished, this,
          [&]()
          {
            checkAndChangeRemoteConfig(
                  ui->cbHost->currentText(),
                  ui->leRemoteDirectory->text() );
          }
  );

  connect(ui->btnSelectLocalWorkingDirectory, &QPushButton::clicked, this,
         [&]()
         {
           QString newDir = QFileDialog::getExistingDirectory(
                 this,
                 "Please select working directory",
                 ui->leLocalWorkingDirectory->text()
                 );
           if (!newDir.isEmpty())
           {
               ui->leLocalWorkingDirectory->setText(newDir);
               changeWorkingDirectory(newDir);
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
                  checkAndChangeRemoteConfig(
                        ui->cbHost->currentText(),
                        QString::fromStdString(dir.string()) );
                  displayCurrentRemoteWorkingDir();
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


void QExecutionWorkspaceDialog::lockRemoteExecution(const QString& reason)
{
    ui->gbPerformRemoteExecution->setChecked(true);
    ui->gbPerformRemoteExecution->setToolTip(reason);
    lockRemoteExecution_=true;
}



boost::filesystem::path QExecutionWorkspaceDialog::localDirectory() const
{
  return ui->leLocalWorkingDirectory->text().toStdString();
}




insight::RemoteLocation* QExecutionWorkspaceDialog::remoteLocation() const
{
  return remoteLocation_.get();
}




QExecutionWorkspaceDialog::~QExecutionWorkspaceDialog()
{
    delete ui;
}

