#include "qexecutionworkspacedialog.h"
#include "ui_qexecutionworkspacedialog.h"

#include "base/wsllinuxserver.h"
#include "base/sshlinuxserver.h"
#include "base/cppextensions.h"
#include "base/translations.h"

#include "base/warningdispatcher.h"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QPointer>
#include <QToolTip>
#include "remotedirselector.h"
#include "iqserverlaunchdialog.h"
#include "iqremoteserverstatus.h"

#include <algorithm>
#include <cmath>
#include <thread>
#include <vector>



using IQRemoteServerStatus::State;




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




void QExecutionWorkspaceDialog::setHostState(const QString& serverLabel, bool online)
{
    hostOnline_[serverLabel]=online;

    int i = ui->cbHost->findText(serverLabel);
    if (i>=0)
    {
        ui->cbHost->setItemIcon(
            i, IQRemoteServerStatus::icon(online ? State::Online : State::Offline) );
        ui->cbHost->setItemData(
            i, online ? _("online") : _("not reachable"), Qt::ToolTipRole );
    }

    updateHostActionButtons();
}




void QExecutionWorkspaceDialog::setHostLoad(
    const QString& serverLabel,
    const insight::RemoteServer::Config::ProcessorLoad& load )
{
    int i = ui->cbHost->findText(serverLabel);
    if (i<0) return;

    if (IQRemoteServerStatus::isBusy(load))
    {
        ui->cbHost->setItemIcon(
            i, IQRemoteServerStatus::icon(State::Busy, load.utilization) );
    }
    ui->cbHost->setItemData(
        i, IQRemoteServerStatus::describe(load), Qt::ToolTipRole );
}




void QExecutionWorkspaceDialog::updateHostActionButtons()
{
    auto label = ui->cbHost->currentText();
    auto i = hostOnline_.find(label);

    // browse remote directories: only for a host known to be online
    {
        bool online = (i!=hostOnline_.end()) && i->second;
        ui->btnSelectRemoteDirectory->setEnabled(online);
        ui->btnSelectRemoteDirectory->setToolTip(
            online ? _("Browse the directories on the host") :
            (i==hostOnline_.end()) ? _("Checking, if the host is reachable...") :
                                     _("The host is not reachable.") );
    }

    // launch: only for a host known to be down, which can be created
    {
        bool creatable=false;
        try
        {
            if (!label.isEmpty())
                creatable = insight::remoteServers().findServer(
                    label.toStdString() )->canBeLaunched();
        }
        catch (...) {}

        if (i==hostOnline_.end())
        {
            ui->btnLaunchHost->setEnabled(false);
            ui->btnLaunchHost->setToolTip(_("Checking, if the host is reachable..."));
        }
        else if (i->second)
        {
            ui->btnLaunchHost->setEnabled(false);
            ui->btnLaunchHost->setToolTip(_("The host is running."));
        }
        else if (!creatable)
        {
            ui->btnLaunchHost->setEnabled(false);
            ui->btnLaunchHost->setToolTip(
                _("The host is not reachable and cannot be launched from here"
                  " (no launch command configured)."));
        }
        else
        {
            ui->btnLaunchHost->setEnabled(true);
            ui->btnLaunchHost->setToolTip(_("Launch the host (execute its launch command)."));
        }
    }

    if (i==hostOnline_.end())
    {
        ui->btnCreateRemoteDirectory->setEnabled(false);
        ui->btnCreateRemoteDirectory->setToolTip(
            _("Checking, if the host is reachable..."));
    }
    else if (!i->second)
    {
        ui->btnCreateRemoteDirectory->setEnabled(false);
        ui->btnCreateRemoteDirectory->setToolTip(
            _("The host is not reachable. A new directory can only be created on a running host."));
    }
    else
    {
        ui->btnCreateRemoteDirectory->setEnabled(true);
        ui->btnCreateRemoteDirectory->setToolTip(
            _("Create a new remote directory with a random name.\n"
              "Unlike the temporary directory, it is kept after the analysis."));
    }
}




void QExecutionWorkspaceDialog::createRemoteDirectory()
{
    try
    {
        auto cfg = insight::remoteServers().findServer(
            ui->cbHost->currentText().toStdString() );

        auto srv = cfg->getInstanceIfRunning();
        if (!srv)
        {
            setHostState(ui->cbHost->currentText(), false);
            throw insight::Exception("The host %s is not reachable.", cfg->c_str());
        }

        // same naming as for temporary storage, see RemoteLocation::initialize
        // (mktemp -d: the directory is created)
        auto dir = srv->getTemporaryDirectoryName(
            cfg->defaultDirectory_/"irXXXXXX" );

        removeCreatedRemoteDirIfUnused(false); // a previously created one
        createdRemoteDir_.reset(new CreatedRemoteDir{cfg, dir});

        checkAndChangeRemoteConfig(
            ui->cbHost->currentText(),
            QString::fromStdString(dir.generic_string()) );
        displayCurrentRemoteWorkingDir();
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(
            this, _("Remote directory creation failed"),
            QString(_("The remote directory could not be created:\n%1"))
                .arg(QString::fromStdString(e.what())) );
    }
}




void QExecutionWorkspaceDialog::removeCreatedRemoteDirIfUnused(bool accepted)
{
    if (!createdRemoteDir_) return;

    bool used =
        accepted
        && ui->gbPerformRemoteExecution->isChecked()
        && ui->cbHost->currentText().toStdString()
               == static_cast<const std::string&>(*createdRemoteDir_->server)
        && boost::filesystem::path(ui->leRemoteDirectory->text().toStdString())
               == createdRemoteDir_->directory;

    if (!used)
    {
        try
        {
            if (auto srv = createdRemoteDir_->server->getInstanceIfRunning())
            {
                insight::dbg()<<"removing unused remote directory "
                               <<createdRemoteDir_->directory<<std::endl;
                srv->removeDirectory(createdRemoteDir_->directory);
            }
        }
        catch (const std::exception& e)
        {
            insight::Warning(
                "could not remove the unused remote directory %s: %s",
                createdRemoteDir_->directory.string().c_str(), e.what() );
        }
    }

    createdRemoteDir_.reset();
}




void QExecutionWorkspaceDialog::checkHostInBackground(
    insight::RemoteServer::ConfigPtr cfg )
{
    auto label = QString::fromStdString(*cfg);
    IQRemoteServerStatus::checkInBackground(
        cfg, this,
        [this,label](bool online) { setHostState(label, online); },
        [this,label](const insight::RemoteServer::Config::ProcessorLoad& l)
        { setHostLoad(label, l); } );
}




void QExecutionWorkspaceDialog::launchHost()
{
    auto label = ui->cbHost->currentText();

    insight::RemoteServer::ConfigPtr cfg;
    try
    {
        cfg = insight::remoteServers().findServer(label.toStdString());
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, _("Launch failed"), QString::fromStdString(e.what()));
        return;
    }

    // shows the output of the launch command; displays errors itself
    bool launched = IQServerLaunchDialog::launchServer(*cfg, this);

    // re-evaluate the host state (and load) in any case
    hostOnline_.erase(label);
    int idx = ui->cbHost->findText(label);
    if (idx>=0)
    {
        ui->cbHost->setItemIcon(idx, IQRemoteServerStatus::icon(State::Unknown));
        ui->cbHost->setItemData(idx, QVariant(), Qt::ToolTipRole);
    }
    updateHostActionButtons();
    checkHostInBackground(cfg);

    if (launched && ui->cbHost->currentText()==label)
    {
        // re-check the remote directory, now that the host is up
        checkAndChangeRemoteConfig(label, ui->leRemoteDirectory->text());
    }
}




void QExecutionWorkspaceDialog::changeWorkingDirectory(const QString& newDir)
{
  boost::filesystem::path newDirPath = newDir.toStdString();

  insight::dbg()<<"set new work dir:" <<newDir.toStdString()<<std::endl;

  if (!isValidWorkingDir(newDirPath))
  {
        QMessageBox::critical(
            this, _("Invalid Working Directory"),
                  QString(_("The selected working directory \"%1\" is invalid.\n"
                            "Please enter a valid working directory!"))
                .arg(newDir) );

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
                insight::remoteServers().findServer(
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
                ui->leRemoteDirectory->setToolTip(_("The remote directory is invalid!"));
                return;
            }
            else
            {
                ui->leRemoteDirectory->setStyleSheet("QLineEdit{background: lightgreen;}");
                ui->leRemoteDirectory->setToolTip(_("Verified."));
            }
        }
        else
        {
            ui->leRemoteDirectory->setStyleSheet("QLineEdit{background: yellow;}");
            ui->leRemoteDirectory->setToolTip(
                _("The remote directory cannot be checked because the server is unreachable yet!\n"
                  "Please make sure that it is existing!"));
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
  setWindowTitle(_("Setup Workspace"));

  // populate controls

  defaultPal_ = ui->leRemoteDirectory->palette();

  // the server list is ordered by pointer: sort alphabetically for display
  std::vector<insight::RemoteServer::ConfigPtr> servers(
      insight::remoteServers().begin(), insight::remoteServers().end() );
  std::sort(
      servers.begin(), servers.end(),
      [](const insight::RemoteServer::ConfigPtr& a,
         const insight::RemoteServer::ConfigPtr& b)
      {
        return QString::fromStdString(*a).compare(
                 QString::fromStdString(*b), Qt::CaseInsensitive ) < 0;
      }
  );

  for (const auto& i: servers)
  {
    auto label = QString::fromStdString(*i);
    ui->cbHost->addItem( IQRemoteServerStatus::icon(State::Unknown), label );

    checkHostInBackground(i);
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

            if (on)
            {
                checkAndChangeRemoteConfig(
                    ui->cbHost->currentText(),
                    ui->leRemoteDirectory->text() );
            }
            else
            {
                remoteLocation_.reset();
            }
          }
  );

  connect(ui->cbHost, &QComboBox::currentTextChanged, this,
          [&](const QString &srv)
          {
            updateHostActionButtons();
            checkAndChangeRemoteConfig(
                  srv,
                  ui->leRemoteDirectory->text() );
          }
  );

  connect(ui->btnCreateRemoteDirectory, &QPushButton::clicked,
          this, &QExecutionWorkspaceDialog::createRemoteDirectory );

  connect(ui->btnLaunchHost, &QPushButton::clicked,
          this, &QExecutionWorkspaceDialog::launchHost );

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
          _("Please select working directory"),
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
            // the button is only enabled, if the host is known to be online:
            // no additional reachability check here
            insight::RemoteServerPtr server;
            try
            {
              QApplication::setOverrideCursor(Qt::WaitCursor);
              server = insight::remoteServers().findServer(
                    ui->cbHost->currentText().toStdString() )->instance();
              QApplication::restoreOverrideCursor();
            }
            catch (const std::exception& e)
            {
              QApplication::restoreOverrideCursor();
              QMessageBox::critical(
                    this, _("Remote directory selection"),
                    QString(_("Could not connect to the host:\n%1"))
                      .arg(QString::fromStdString(e.what())) );
              return;
            }

            {
              {
                RemoteDirSelector dlg(this, server);

                if (dlg.exec()==QDialog::Accepted)
                {
                  auto dir=dlg.selectedRemoteDir().generic_path();

                 checkAndChangeRemoteConfig(
                        ui->cbHost->currentText(),
                        QString::fromStdString(dir.string()) );
                  displayCurrentRemoteWorkingDir();
                }
              }
            }
          }
  );

  connect(this, &QDialog::rejected, this,
          [&]()
          {
            removeCreatedRemoteDirIfUnused(false);
          }
  );

  connect(this, &QDialog::accepted, this,
          [&]()
          {
            removeCreatedRemoteDirIfUnused(true);

            if (ui->gbPerformRemoteExecution->isChecked())
            {
              remoteLocation_.reset(
                    new insight::RemoteLocation(
                      insight::remoteServers().findServer(
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
    // remoteLocation_ will be set by signal handler above
    ui->leLocalWorkingDirectory->setText(
          QString::fromStdString(
              localDirectory->string() ) );
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

  updateHostActionButtons();
}


void QExecutionWorkspaceDialog::lockRemoteExecution(const QString& reason)
{
    ui->gbPerformRemoteExecution->setChecked(true);
    ui->gbPerformRemoteExecution->setToolTip(reason);
    lockRemoteExecution_=true;
}




void QExecutionWorkspaceDialog::restrictToRemoteLocation(
    const boost::filesystem::path& localDirectory )
{
    // stays checkable (isChecked() is always false otherwise), but locked
    ui->gbPerformRemoteExecution->setChecked(true);
    ui->gbPerformRemoteExecution->setTitle(_("Remote location"));
    lockRemoteExecution_=true;

    ui->label->hide();
    ui->leLocalWorkingDirectory->hide();
    ui->btnSelectLocalWorkingDirectory->hide();

    // setChecked(true) above might not have emitted toggled, if it was checked already
    if (!remoteLocation_)
    {
        checkAndChangeRemoteConfig(
            ui->cbHost->currentText(),
            ui->leRemoteDirectory->text() );
    }

    setWindowTitle(
        QString(_("Setup Remote Location for %1"))
            .arg(QString::fromStdString(
                boost::filesystem::absolute(localDirectory).string() ) ) );
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

