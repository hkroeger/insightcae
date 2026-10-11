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

#include <algorithm>
#include <thread>
#include <vector>



namespace {

enum class HostState { Unknown, Online, Offline };

QIcon hostStateIcon(HostState state)
{
    QPixmap pm(12, 12);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r(1.5, 1.5, 9., 9.);
    switch (state)
    {
    case HostState::Online:
        p.setPen(QPen(QColor("#1d6b2d"), 1.));
        p.setBrush(QColor("#2e9e44"));
        break;
    case HostState::Offline:
        p.setPen(QPen(QColor("#404040"), 1.5));
        p.setBrush(Qt::NoBrush);
        break;
    case HostState::Unknown:
        p.setPen(QPen(QColor("#a0a0a0"), 1., Qt::DotLine));
        p.setBrush(Qt::NoBrush);
        break;
    }
    p.drawEllipse(r);

    return QIcon(pm);
}

}




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
            i, hostStateIcon(online ? HostState::Online : HostState::Offline) );
        ui->cbHost->setItemData(
            i, online ? _("online") : _("not reachable"), Qt::ToolTipRole );
    }

    updateCreateRemoteDirButton();
}




void QExecutionWorkspaceDialog::updateCreateRemoteDirButton()
{
    auto i = hostOnline_.find(ui->cbHost->currentText());
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
    ui->cbHost->addItem( hostStateIcon(HostState::Unknown), label );

    // check reachability in background (ssh connect may take some seconds)
    QPointer<QExecutionWorkspaceDialog> guard(this);
    insight::RemoteServer::ConfigPtr cfg = i;
    std::thread(
        [cfg, label, guard]()
        {
            bool online=false;
            try
            {
                online = cfg->isRunning();
            }
            catch (const std::exception& e)
            {
                insight::dbg()<<"checking server "<<label.toStdString()
                               <<" failed: "<<e.what()<<std::endl;
            }
            QMetaObject::invokeMethod(
                qApp,
                [guard, label, online]()
                {
                    if (guard) guard->setHostState(label, online);
                },
                Qt::QueuedConnection );
        }
    ).detach();
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
            updateCreateRemoteDirButton();
            checkAndChangeRemoteConfig(
                  srv,
                  ui->leRemoteDirectory->text() );
          }
  );

  connect(ui->btnCreateRemoteDirectory, &QPushButton::clicked,
          this, &QExecutionWorkspaceDialog::createRemoteDirectory );

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
            if (remoteLocation_)
            {
              if (auto server = remoteLocation_->serverConfig().getInstanceIfRunning())
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

  updateCreateRemoteDirButton();
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

