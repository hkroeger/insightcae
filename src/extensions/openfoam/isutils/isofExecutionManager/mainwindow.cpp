#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cstdlib>

#include "base/boost_include.h"
#include "base/tools.h"

#include "openfoam/solveroutputanalyzer.h"

#include <QtGlobal>
#include <QDebug>
#include <QProcess>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <QTimer>
#include <QStatusBar>
#include <QDateTime>
#include <QInputDialog>
#include <QDir>
#include <QHeaderView>

#include "base/qt5_helper.h"

#include "iqremoteparaviewdialog.h"
#include "iqremoteservereditdialog.h"
#include "iqremoteserverstatus.h"
#include "iqserverlaunchdialog.h"
#include "remotesync.h"
#include "base/sshlinuxserver.h"
#include "base/warningdispatcher.h"

#include <QApplication>


void IQRXRemoteExecutionState::updateGUI(bool enabled)
{
  if (auto *mw = dynamic_cast<MainWindow*>(parent()))
  {
      auto *ui = mw->ui;
      insight::assertion(ui, "internal error: user interface record is unset");

      if (enabled)
      {
        insight::assertion(bool(rlc_), "internal error: remote location is unset");

        ui->server->setText(QString::fromStdString(rlc_->serverLabel()));
        ui->remoteDir->setText(QString::fromStdString(rlc_->remoteDir().string()));

        if (isCommitted())
        {
          bfs_path loc_dir=boost::filesystem::absolute(exeConfig().localDir());
          mw->setWindowTitle(QString::fromStdString(loc_dir.string())+" - InsightCAE Execution Manager");

#ifndef NO_TERMWIDGET
        mw->terminal_->changeDir( ui->localDir->text() );
        if ( std::shared_ptr<insight::SSHLinuxServer> ssh =
                std::dynamic_pointer_cast<insight::SSHLinuxServer>(exeConfig().server()) )
        {
          auto cmd = QString("ssh %1 -t 'cd '%2'; bash -l'\n")
              .arg(QString::fromStdString(ssh->SSHServerConfig().hostName_))
              .arg(QString::fromStdString(rlc_->remoteDir().string()))
              ;
          mw->terminal_->sendText(cmd);
        }
#endif

          mw->tsi_.reset(new insight::TaskSpoolerInterface(
                       exeConfig().socket(), exeConfig().server()));
          mw->onRefreshJobList();
          mw->setRemoteFilesView(exeConfig().server(), rlc_->remoteDir());
        }
        else
        {
          // not connected (e.g. host stopped): no access to the job queue
          mw->setWindowTitle( ui->localDir->text() + " - InsightCAE Execution Manager" );
          mw->tsi_.reset();
          ui->commands->clear();
          mw->setRemoteFilesView(nullptr);
        }
      }
      else
      {
        // this state is destroyed (deferred): don't touch the GUI,
        // if it has been replaced by another state meanwhile
        if (mw->remote_ && mw->remote_.data()!=this) return;

        mw->setWindowTitle( ui->localDir->text() + " - InsightCAE Execution Manager" );
        mw->tsi_.reset();
        ui->commands->clear();
        mw->setRemoteFilesView(nullptr);
        ui->server->setText("(unset)");
        ui->remoteDir->setText("(unset)");
      }
  }
//  else insight::assertion(mw, "internal error: main window is unset");

}


//void MainWindow::updateGUI()
//{
//  setWindowTitle( ui->localDir->text() + " - InsightCAE Execution Manager" );

//  if (remote_)
//  {
//    bfs_path loc_dir=boost::filesystem::absolute(remote_->localDir());
//    setWindowTitle(QString::fromStdString(loc_dir.string())+" - InsightCAE Execution Manager");
//    ui->server->setText(QString::fromStdString(remote_->server()->serverLabel()));
//    ui->localDir->setText(QString::fromStdString(loc_dir.string()));
//    ui->remoteDir->setText(QString::fromStdString(remote_->remoteDir().string()));

//#ifndef NO_TERMWIDGET
//    terminal_->changeDir( ui->localDir->text() );
//    if ( std::shared_ptr<insight::SSHLinuxServer> ssh =
//            std::dynamic_pointer_cast<insight::SSHLinuxServer>(remote_->server()) )
//    {
//      auto cmd = QString("ssh %1 -t 'cd '%2'; bash -l'\n")
//          .arg(QString::fromStdString(ssh->serverConfig()->hostName_))
//          .arg(QString::fromStdString(remote_->remoteDir().string()))
//          ;
//      terminal_->sendText(cmd);
//    }
//#endif

//    tsi_.reset(new insight::TaskSpoolerInterface(remote_->socket(), remote_->server()));
//    onRefreshJobList();
//  }
//}




JobListBuilder::JobListBuilder(std::shared_ptr<insight::TaskSpoolerInterface> tsi)
  : tsi_(tsi)
{}




void JobListBuilder::run()
{
  auto result = tsi_->jobs();
  emit jobListReady(result);
}




void MainWindow::onRefreshJobList()
{
  if (tsi_)
  {
    //auto jl = tsi_->jobs();

    if (!jbl_thread_)
    {
      jbl_thread_ = new JobListBuilder(tsi_);
      connect(jbl_thread_, &JobListBuilder::jobListReady, this, &MainWindow::onJobListReady);
      connect(jbl_thread_, &JobListBuilder::finished,
              jbl_thread_,
              [&]() {
                  jbl_thread_->deleteLater();
                  jbl_thread_=nullptr;
              }
      );
      jbl_thread_->start();
    }
  }
}




void MainWindow::onJobListReady(insight::TaskSpoolerInterface::JobList jl)
{
  ui->commands->clear();

  int num=0;
  for (auto j: jl)
  {
    if (j.state!=insight::TaskSpoolerInterface::Finished)
    {
      ui->commands->addItem( QString::number(j.id) +" "+ QString::fromStdString(j.commandLine) );
      num++;
    }
  }

  if ( (num>0) && !tsi_->isTailRunning() )
  {
    onStartTail();
  }
}




void MainWindow::onStartTail()
{
  if (tsi_)
  {
    soa_.reset(new insight::SolverOutputAnalyzer(*ui->graph)); // reset

    tsi_->startTail(
          [&](std::string line) {
            emit logReady(QString::fromStdString(line));
          }
    );
  }
}




void MainWindow::updateOutputAnalzer(QString line)
{
  soa_->update(line.toStdString());
}


bool isFixedPitch(const QFont &font) {
   const QFontInfo fi(font);
//   qDebug() << fi.family() << fi.fixedPitch();
   return fi.fixedPitch();
}

QFont getMonospaceFont() {
  QFont font("fixed");
  if (isFixedPitch(font)) return font;
  font.setFamily("monospace");
  if (isFixedPitch(font)) return font;
  font.setStyleHint(QFont::Monospace);
  if (isFixedPitch(font)) return font;
  font.setStyleHint(QFont::TypeWriter);
  if (isFixedPitch(font)) return font;
  return font;
}



//void myMessageHandler(QtMsgType, const QMessageLogContext &, const QString &msg)
//{
//    qDebug()<<msg;
//}

MainWindow::MainWindow(const boost::filesystem::path& location, QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
//     qInstallMessageHandler(&myMessageHandler);

  ui->setupUi(this);

  ui->localDir->setText( QString::fromStdString(boost::filesystem::absolute(location).string()) );

  // file trees in the status tab
  localFilesModel_ = new QFileSystemModel(this);
  localFilesModel_->setFilter(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden);
  ui->local_files->setModel(localFilesModel_);
  ui->local_files->setRootIndex( localFilesModel_->setRootPath(ui->localDir->text()) );
  ui->local_files->hideColumn(2); // type
  ui->local_files->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  ui->local_files->header()->setStretchLastSection(false);
  ui->btn_refresh_remote_files->setEnabled(false); // until connected

  // state of the remote host and actions on it (in the server row)
  serverStatusIcon_ = new QLabel;
  ui->horizontalLayout->addWidget(serverStatusIcon_);
  serverStatus_ = new QLabel;
  serverStatus_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  ui->horizontalLayout->addWidget(serverStatus_, 1);
  btnLaunch_ = new QPushButton("Launch");
  btnShutdown_ = new QPushButton("Shutdown");
  btnDispose_ = new QPushButton("Shutdown && Dispose");
  btnRefreshStatus_ = new QPushButton("Refresh status");
  btnRefreshStatus_->setToolTip("Check the state of the remote host again");
  for (auto *b: {btnLaunch_, btnShutdown_, btnDispose_, btnRefreshStatus_})
    ui->horizontalLayout->addWidget(b);
  connect(btnLaunch_, &QPushButton::clicked, this, &MainWindow::launchServer);
  connect(btnShutdown_, &QPushButton::clicked, this, &MainWindow::shutdownServer);
  connect(btnDispose_, &QPushButton::clicked, this, &MainWindow::disposeServer);
  connect(btnRefreshStatus_, &QPushButton::clicked, this, &MainWindow::checkServerStatus);

  // remote server configuration (same dialog as in the workbench)
  {
    ui->menuRemote->addSeparator();
    auto *a = ui->menuRemote->addAction("Remote servers...");
    connect(a, &QAction::triggered, this,
            [this]()
            {
              IQRemoteServerEditDialog dlg(this);
              dlg.exec();
              checkServerStatus();
            } );
  }
  connect(ui->actionRemove_remote_directory, &QAction::triggered,
          this, &MainWindow::removeRemoteDirectory );

  updateServerControls();


  setWindowIcon(QIcon(":/resources/logo_insight_cae.svg"));
  this->setWindowTitle("InsightCAE Execution Manager");

  soa_.reset(new insight::SolverOutputAnalyzer(*ui->graph));

  connect(
      ui->actionSelect_remote_server, &QAction::triggered, this,
      [&]()
      {
        QStringList servers;
          for (const auto& s: insight::remoteServers())
        {
          servers<<QString::fromStdString(*s);
        }

        int currentServerIdx = 0;
        if (remote_)
        {
          currentServerIdx =
              servers.indexOf(QString::fromStdString(remote_->location().serverLabel()));
        }
        bool ok = false;

        auto selectedServer = QInputDialog::getItem(
              this,
              "Select remote server",
              "Please select the remote server:", servers,
              currentServerIdx, false, &ok);

        if (ok && !selectedServer.isEmpty())
        {
          setRemote( IQRemoteExecutionState::New<IQRXRemoteExecutionState>(
                this,
                insight::remoteServers().findServer(
                  selectedServer.toStdString() )
                ) );
          remote_->location().writeConfigFile(
                insight::RemoteExecutionConfig::defaultConfigFile("."));

        }
      }
  );

  connect(
      ui->actionSelect_Remote_Directory, &QAction::triggered, this,
      [&]()
      {
          if (remote_)
          {
              // the host has to be running for browsing its directories
              const auto& scfg = remote_->location().serverConfig();
              if (!scfg.isRunning() && scfg.canBeLaunched())
              {
                  if (!IQServerLaunchDialog::launchServer(scfg, this))
                      return; // error was displayed
                  checkServerStatus();
              }
              auto server = scfg.getInstanceIfRunning();
              if (!server)
              {
                  QMessageBox::critical(
                      this, "Server not reachable",
                      "The remote server is not reachable. Please make sure, that it is running.");
                  return;
              }

              RemoteDirSelector dlg(this, server);

              if (dlg.exec() == QDialog::Accepted)
              {

                  ui->remoteDir->setText( QString::fromStdString(dlg.selectedRemoteDir().string()) );

                  setRemote( IQRemoteExecutionState::New<IQRXRemoteExecutionState>(
                      this,
                      remote_->location().serverConfig().clone(),
                      dlg.selectedRemoteDir()
                      ) );
                  remote_->location().writeConfigFile(
                      insight::RemoteExecutionConfig::defaultConfigFile("."));
              }
          }
      }
  );

  connect(ui->action_syncLocalToRemote, &QAction::triggered, this,
          [this]() { syncLocalToRemote(false); } );
  connect(ui->action_syncRemoteToLocal, &QAction::triggered, this,
          [this]() { syncRemoteToLocal(false); } );
  connect(ui->actionRemote_Local_restart_periodically, &QAction::triggered, this,
          [this]()
          {
              bool ok;
              int seconds=QInputDialog::getInt(
                  this, "Time interval for auto restart",
                  "Please enter time interval from transfer end to next restart:",
                  300, -1, INT_MAX, 1, &ok);
              if (ok)
              {
                syncRemoteToLocal(false, seconds);
              }
          }
  );

  connect(ui->action_syncLocalToRemote_includeprocessor, &QAction::triggered, this,
          [this]() { syncLocalToRemote(true); } );
  connect(ui->action_syncRemoteToLocal_includeprocessor, &QAction::triggered, this,
          [this]() { syncRemoteToLocal(true); } );
  connect(ui->actionRemote_Local_incl_proc_dirs_restart_periodically, &QAction::triggered, this,
          [this]()
          {
              bool ok;
              int seconds=QInputDialog::getInt(
                  this, "Time interval for auto restart",
                  "Please enter time interval from transfer end to next restart:",
                  300, -1, INT_MAX, 1, &ok);
              if (ok)
              {
                  syncRemoteToLocal(true, seconds);
              }
          }
  );

  setBWLimit(-1);
  connect(ui->action_setbwlimit, &QAction::triggered, this,
          [&]()
          {
              if (remote_)
              {
                  bool ok=false;
                  int newlimit = QInputDialog::getInt(
                              this,
                              "Bandwidth limit",
                              "Enter bandwidth limit (kB/s)",
                              11000, -1,
                              2147483647, 1000,
                              &ok);
                  if (ok) setBWLimit(newlimit);
              }
          }
  );

  // transfer the selected items or (if nothing is selected) everything
  connect(ui->sync_to_remote, &QPushButton::clicked, this,
          [this]() { syncLocalToRemote(false, localSelection()); } );
  connect(ui->sync_to_local, &QPushButton::clicked, this,
          [this]() { syncRemoteToLocal(false, -1, remoteSelection()); } );
  connect(ui->btn_refresh_remote_files, &QPushButton::clicked, this,
          [this]()
          {
            if (remoteFilesModel_)
              remoteFilesModel_->refreshBelow(QModelIndex()); // in background
          } );
  connect(ui->actionRemote_write_and_copy_to_local, &QAction::triggered, this,
          [=]() {
            this->remoteWriteAndCopyBack(false);
          });
  connect(ui->actionRemote_write_reconstruct_and_copy_to_local, &QAction::triggered, this,
          [=]() {
            this->remoteWriteAndCopyBack(true);
          });

  connect(ui->actionStart_Paraview, &QAction::triggered, this, &MainWindow::onStartParaview);
  connect(ui->actionStart_Remote_Paraview, &QAction::triggered, this, &MainWindow::onStartRemoteParaview);

  connect(this, &MainWindow::logReady, ui->log, &LogViewerWidget::appendLine);
  connect(this, &MainWindow::logReady, this, &MainWindow::updateOutputAnalzer ); // through signal/slot to execute analysis in GUI thread

#ifndef NO_TERMWIDGET
  terminal_ = new QTermWidget( 1, ui->tabWidget );

  QFont font = getMonospaceFont();
  terminal_->setTerminalFont(font);

  ui->tabWidget->addTab(terminal_, "&4 - Terminal");
#endif

  connect(ui->btn_refresh, &QPushButton::clicked, this, &MainWindow::onRefreshJobList);
  connect(ui->btn_kill, &QPushButton::clicked, this,
          [&]() {
            if (tsi_) {
              tsi_->kill();
              tsi_->stopTail();
              onRefreshJobList();
            }
          }
  );

  connect(ui->btn_clean, &QPushButton::clicked, this,
          [&]() {
            if (tsi_) {
              tsi_->clean();
              onRefreshJobList();
            }
          }
  );

  connect(ui->btn_scroll, &QPushButton::clicked, ui->log, &LogViewerWidget::autoScrollLog);
  connect(ui->btn_clear, &QPushButton::clicked, ui->log, &LogViewerWidget::clearLog);
  connect(ui->btn_save, &QPushButton::clicked, ui->log, &LogViewerWidget::saveLog);
  connect(ui->btn_email, &QPushButton::clicked, ui->log, &LogViewerWidget::sendLog);

  connect(ui->btn_clear_charts, &QPushButton::clicked, this, &MainWindow::onClearProgressCharts);

  refreshTimer_ = new QTimer(this);
  connect(refreshTimer_, &QTimer::timeout, this, &MainWindow::onRefreshJobList);
  refreshTimer_->start(5000);

//  auto sbw = new QWidget;
//  sbw->setLayout( statusBarLayout_ = new QHBoxLayout );
////  statusBar()->addPermanentWidget(sbw, 1);
//  statusBarLayout_->addStretch();

  auto reccfg = insight::RemoteExecutionConfig::defaultConfigFile(location);
  if ( boost::filesystem::exists( reccfg ) )
  {
    try
    {
      // don't validate here: the host might be switched off.
      // The connection is established on demand.
      setRemote( IQRemoteExecutionState::New<IQRXRemoteExecutionState>(
            this, reccfg, true ) );
      if (remote_->location().tryConnect())
        remote_->commit(location);
    }
    catch (const std::exception& e)
    {
      insight::Warning(
          "The remote execution configuration %s could not be read and is ignored.\n"
          "Reason: %s", reccfg.string().c_str(), e.what() );
    }
  }

  readSettings(); // after remote location is set

}




void MainWindow::setRemote(IQRemoteExecutionState* newRemote)
{
  if (remote_ && remote_.data()!=newRemote)
  {
    auto *old = remote_.data();
    remote_ = nullptr;
    old->discard();
  }
  remote_ = newRemote;
  checkServerStatus();
}




void MainWindow::setRemoteFilesView(
    insight::RemoteServerPtr server,
    const boost::filesystem::path& remoteDir )
{
  if ( server==remoteFilesServer_ && remoteDir==remoteFilesDir_
       && bool(remoteFilesModel_)==bool(server) )
    return; // unchanged, keep the expanded state

  remoteFilesServer_ = server;
  remoteFilesDir_ = remoteDir;

  auto *oldModel = remoteFilesModel_;
  auto *oldSelectionModel = ui->remote_files->selectionModel();

  remoteFilesModel_ = nullptr;
  if (server)
  {
    remoteFilesModel_ = new IQRemoteFolderModel(this, server, remoteDir, true);
    auto serverLabel = QString::fromStdString(server->serverLabel());
    connect(remoteFilesModel_, &IQRemoteFolderModel::listingStateChanged, this,
            [this,serverLabel](int pending, const QString& path)
            {
              if (pending>0)
                statusBar()->showMessage(
                    QString("Listing %1 on %2...").arg(path, serverLabel) );
              else
                statusBar()->clearMessage();
            } );
  }

  ui->remote_files->setModel(remoteFilesModel_);
  ui->btn_refresh_remote_files->setEnabled(bool(remoteFilesModel_));
  delete oldSelectionModel;
  if (oldModel) oldModel->deleteLater();

  if (remoteFilesModel_)
    remoteFilesModel_->fetchMore(QModelIndex()); // in background
}




std::vector<boost::filesystem::path> MainWindow::localSelection() const
{
  std::vector<boost::filesystem::path> sel;
  QDir base(ui->localDir->text());
  for (const auto& i: ui->local_files->selectionModel()->selectedRows(0))
  {
    sel.push_back(
        base.relativeFilePath(localFilesModel_->filePath(i)).toStdString() );
  }
  return sel;
}




std::vector<boost::filesystem::path> MainWindow::remoteSelection() const
{
  std::vector<boost::filesystem::path> sel;
  if (remoteFilesModel_)
  {
    for (const auto& i: ui->remote_files->selectionModel()->selectedRows(0))
    {
      if (remoteFilesModel_->isEntry(i))
        sel.push_back(remoteFilesModel_->relativePath(i));
    }
  }
  return sel;
}




bool MainWindow::ensureConnected()
{
  if (!remote_)
  {
    QMessageBox::critical(
        this, "No remote location",
        "There is no remote location configured.\n"
        "Please select a remote server and directory first." );
    return false;
  }

  if (remote_->isCommitted() && remote_->location().isActive())
    return true;

  const auto& scfg = remote_->location().serverConfig();
  if (scfg.canBeLaunched() && !scfg.isRunning())
  {
    // shows the output of the launch (incl. creation); displays errors itself
    if (!IQServerLaunchDialog::launchServer(scfg, this))
    {
      checkServerStatus();
      return false;
    }
  }

  try
  {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    remote_->commit( ui->localDir->text().toStdString() );
    QApplication::restoreOverrideCursor();
  }
  catch (const std::exception& e)
  {
    QApplication::restoreOverrideCursor();
    QMessageBox::critical(
        this, "Connection failed",
        QString("Could not connect to the remote location:\n%1")
          .arg(QString::fromStdString(e.what())) );
    checkServerStatus();
    return false;
  }

  setBWLimit(bwLimit_); // apply to the connected server
  checkServerStatus();

  if (!remote_->location().isActive())
  {
    QMessageBox::critical(
        this, "Remote directory not available",
        QString("The remote directory %1 does not exist.")
          .arg(QString::fromStdString(remote_->location().remoteDir().string())) );
    return false;
  }
  return true;
}




void MainWindow::checkServerStatus()
{
  serverOnline_.reset();
  serverExists_.reset();
  serverLoad_.reset();
  int gen = ++statusGeneration_;
  updateServerControls();

  if (!remote_) return;

  IQRemoteServerStatus::checkInBackground(
      remote_->location().serverConfig().clone(), this,
      [this,gen](bool online)
      {
        if (gen!=statusGeneration_) return;
        serverOnline_=online;
        updateServerControls();
      },
      [this,gen](const insight::RemoteServer::Config::ProcessorLoad& l)
      {
        if (gen!=statusGeneration_) return;
        serverLoad_=l;
        updateServerControls();
      },
      [this,gen](bool exists)
      {
        if (gen!=statusGeneration_) return;
        serverExists_=exists;
        updateServerControls();
      } );
}




void MainWindow::updateServerControls()
{
  using IQRemoteServerStatus::State;

  // status display
  State st = State::Unknown;
  QString txt;
  if (!remote_)
  {
    txt = QString();
  }
  else if (!serverOnline_)
  {
    txt = "checking...";
  }
  else if (*serverOnline_)
  {
    st = State::Online;
    txt = "online";
    if (serverLoad_)
    {
      if (IQRemoteServerStatus::isBusy(*serverLoad_)) st = State::Busy;
      txt = IQRemoteServerStatus::describe(*serverLoad_);
    }
    if (!remote_->isCommitted())
      txt += " (not connected)";
  }
  else
  {
    st = State::Offline;
    txt = serverExists_ ?
            ( *serverExists_ ? "not reachable (instance exists)" : "instance does not exist" ) :
            "not reachable";
  }
  serverStatusIcon_->setVisible(bool(remote_));
  serverStatusIcon_->setPixmap(
      IQRemoteServerStatus::icon(
          st, serverLoad_ ? serverLoad_->utilization : 0. ).pixmap(12, 12) );
  serverStatus_->setText(txt);

  // actions (same rules as in the remote servers dialog)
  const insight::RemoteServer::Config* cfg =
      remote_ ? &remote_->location().serverConfig() : nullptr;
  bool known = bool(serverOnline_);
  bool online = known && *serverOnline_;
  bool exists = (serverExists_ && *serverExists_);
  const QString checking = "Checking, if the server is reachable...";

  btnLaunch_->setEnabled( cfg && known && !online && cfg->canBeLaunched() );
  btnLaunch_->setToolTip(
      !cfg ? "No remote server selected." :
      !known ? checking :
      online ? "The server is running." :
      !cfg->canBeLaunched() ?
        "The server cannot be launched from here (no launch or creation command configured)." :
      (serverExists_ && !*serverExists_) ?
        "Create the instance and launch the server (execute its creation command)." :
        "Launch the server (execute its launch command; create its instance before, if it does not exist)." );

  btnShutdown_->setEnabled( cfg && online && cfg->canBeShutDown() );
  btnShutdown_->setToolTip(
      !cfg ? "No remote server selected." :
      !known ? checking :
      !online ? "The server is not running." :
      !cfg->canBeShutDown() ?
        "The server cannot be shut down from here (no shutdown command configured)." :
        "Shut down the server (execute its shutdown command)." );

  btnDispose_->setEnabled( cfg && cfg->canBeDisposed() && (online || exists) );
  btnDispose_->setToolTip(
      !cfg ? "No remote server selected." :
      !cfg->canBeDisposed() ?
        "The server instance cannot be disposed from here (no destruction command configured)." :
      !known ? checking :
      (serverExists_ && !*serverExists_) ? "The instance does not exist." :
      !(online || exists) ? "Checking, if the instance exists..." :
        "Shut down the server (if running) and delete its instance (execute its destruction command).\n"
        "The remote directory and the local remote configuration are removed as well." );

  btnRefreshStatus_->setEnabled( bool(remote_) );
  ui->actionRemove_remote_directory->setEnabled( bool(remote_) );
}




void MainWindow::launchServer()
{
  if (!remote_) return;
  if (IQServerLaunchDialog::launchServer(remote_->location().serverConfig(), this)) // displays errors itself
  {
    // connect: job list and terminal become available
    ensureConnected();
  }
  checkServerStatus();
}




void MainWindow::shutdownServer()
{
  if (!remote_) return;
  auto label = QString::fromStdString(remote_->location().serverLabel());

  auto answer = QMessageBox::question(
      this, "Shut down server",
      QString("Shut down the server %1?\n\n"
              "Analyses, which are currently running on it, will be terminated"
              " and data on its local storage might be lost.").arg(label),
      QMessageBox::Yes|QMessageBox::No, QMessageBox::No );
  if (answer!=QMessageBox::Yes) return;

  if (tsi_) tsi_->stopTail();
  if (IQServerLaunchDialog::shutdownServer(remote_->location().serverConfig(), this))
  {
    remote_->serverStopped(); // not connected anymore, the job queue is dropped
  }
  checkServerStatus();
}




void MainWindow::disposeServer()
{
  if (!remote_) return;
  auto label = QString::fromStdString(remote_->location().serverLabel());

  auto answer = QMessageBox::warning(
      this, "Dispose server",
      QString("Shut down the server %1 and delete its instance?\n\n"
              "Analyses, which are currently running on it, will be terminated"
              " and all data on the instance will be deleted!\n"
              "The local remote configuration (meta.foam) is removed as well.").arg(label),
      QMessageBox::Yes|QMessageBox::No, QMessageBox::No );
  if (answer!=QMessageBox::Yes) return;

  if (tsi_) tsi_->stopTail();
  if (IQServerLaunchDialog::disposeServer(remote_->location().serverConfig(), this))
  {
    // the instance (incl. the remote directory) does not exist anymore
    try
    {
      boost::filesystem::remove(
          insight::RemoteExecutionConfig::defaultConfigFile(
              ui->localDir->text().toStdString() ) );
    }
    catch (const std::exception& e)
    {
      insight::Warning("Could not remove the remote configuration file: %s", e.what());
    }
    setRemote(nullptr);
  }
  else
  {
    checkServerStatus();
  }
}




void MainWindow::removeRemoteDirectory()
{
  if (!remote_) return;

  auto answer = QMessageBox::question(
      this, "Remove remote directory",
      QString("The directory %1 on server %2 and its contents will be deleted!\n"
              "The local remote configuration (meta.foam) is removed as well.\n"
              "Continue?")
        .arg(QString::fromStdString(remote_->location().remoteDir().string()))
        .arg(QString::fromStdString(remote_->location().serverLabel())),
      QMessageBox::Yes|QMessageBox::No, QMessageBox::No );
  if (answer!=QMessageBox::Yes) return;

  if (!ensureConnected()) return;

  if (tsi_) tsi_->stopTail();
  remote_->cleanup(true); // removes the remote directory and meta.foam, discards the state
  remote_ = nullptr;
  checkServerStatus();
}




MainWindow::~MainWindow()
{
  if (tsi_)
  {
    tsi_->stopTail();
  }

  auxJobThread_.quit();
  auxJobThread_.wait();

  delete ui;
}





void MainWindow::syncLocalToRemote(
    bool includeProcDirs,
    const std::vector<boost::filesystem::path>& selection )
{
  if (ensureConnected())
  {
    auto *rstr = new insight::RunSyncToRemote(
                remote_->exeConfig(),
                includeProcDirs,
                selection );

    auto pb = new QProgressBar;
    statusBar()->addPermanentWidget(pb);
    connect(rstr, &QObject::destroyed, pb, &QProgressBar::deleteLater);

    connect(rstr, &insight::RunSyncToRemote::progressValueChanged, pb, &QProgressBar::setValue);
    connect(rstr, &insight::RunSyncToRemote::progressTextChanged,
            std::bind(&QStatusBar::showMessage, statusBar(), std::placeholders::_1, 0) );
    connect(rstr, &insight::RunSyncToRemote::transferFinished, this,
            [&]()
            {
              statusBar()->showMessage("Transfer to remote location finished");
              // show the new remote contents
              if (remoteFilesModel_)
                remoteFilesModel_->refreshBelow(QModelIndex());
            }
    );
    connect(rstr, &insight::RunSyncToRemote::transferFinished, rstr, &QObject::deleteLater);

    auto stopBtn = new QPushButton("STOP");
    statusBar()->addPermanentWidget(stopBtn);
    connect(stopBtn, &QPushButton::clicked, rstr,
            std::bind(&insight::RunSyncToLocal::interrupt, rstr) );
    connect(rstr, &insight::RunSyncToRemote::transferFinished,
            stopBtn, &QObject::deleteLater);

    statusBar()->showMessage(
        selection.empty() ?
            QString("Transfer to remote location started") :
            QString("Transfer of %1 selected item(s) to remote location started")
              .arg(selection.size()) );

    rstr->start();
  }
}





void MainWindow::syncRemoteToLocal(
    bool includeProcDirs, int autoRestartSeconds,
    const std::vector<boost::filesystem::path>& selection )
{
  if (ensureConnected())
  {
    auto* rstl = new insight::RunSyncToLocal(
                remote_->exeConfig(),
                includeProcDirs,
                selection );

    auto pb = new QProgressBar;
    statusBar()->addPermanentWidget(pb);
    connect(rstl, &QObject::destroyed, pb, &QProgressBar::deleteLater);

    connect(rstl, &insight::RunSyncToLocal::progressValueChanged,
            pb, &QProgressBar::setValue);
    connect(rstl, &insight::RunSyncToLocal::progressTextChanged,
            std::bind(&QStatusBar::showMessage, statusBar(), std::placeholders::_1, 0) );
    connect(rstl, &insight::RunSyncToLocal::transferFinished, rstl, &QObject::deleteLater);
    connect(rstl, &insight::RunSyncToLocal::transferFinished, rstl,
            [&]()
            {
              statusBar()->showMessage(
                  QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss.z : ")
                  +"Transfer from remote location to local directory finished" );
            }
    );
    auto stopBtn = new QPushButton("STOP");
    statusBar()->addPermanentWidget(stopBtn);
    connect(stopBtn, &QPushButton::clicked, rstl,
            std::bind(&insight::RunSyncToLocal::interrupt, rstl) );
    connect(rstl, &insight::RunSyncToLocal::transferFinished,
            stopBtn, &QObject::deleteLater);

    statusBar()->showMessage(
        selection.empty() ?
            QString("Transfer from remote location to local directory started") :
            QString("Transfer of %1 selected item(s) from remote location to local directory started")
              .arg(selection.size()) );

    if (autoRestartSeconds>0)
    {

      connect(rstl, &insight::RunSyncToLocal::transferFinished, rstl,
          [this,includeProcDirs,autoRestartSeconds,selection]()
          {
            auto timer = new QTimer(this);
            timer->setSingleShot(true);
            connect(timer, &QTimer::timeout, this,
              std::bind(&MainWindow::syncRemoteToLocal, this, includeProcDirs, autoRestartSeconds, selection)
              );
            connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);

            auto stopBtn = new QPushButton("STOP");
            statusBar()->addPermanentWidget(stopBtn);
            connect(stopBtn, &QPushButton::clicked, timer, &QObject::deleteLater);
            connect(timer, &QTimer::destroyed, stopBtn, &QObject::deleteLater);

            timer->start(1000*autoRestartSeconds);
          }
      );
    }

    rstl->start();

  }
}




void MainWindow::setBWLimit(int bwlimit)
{
    QString label = "Set bandwidth limit";
    bwLimit_ = bwlimit;

    {
        // applied to the server now, if connected, otherwise when connecting
        if (remote_ && remote_->isCommitted())
        {
            auto srv = remote_->exeConfig().server();
            srv->setTransferBandWidthLimit(bwlimit);
        }

        if (bwlimit>0)
        {
            label = QString("%1 (%2)")
                .arg(label)
                .arg(bwlimit);
        }
        else
        {
            label = label+" (unlimited)";
        }
    }

    ui->action_setbwlimit->setText(label);
}




void MainWindow::onStartParaview()
{
  if (!QProcess::startDetached("isPV.py", QStringList(), ui->localDir->text() ))
  {
    QMessageBox::critical(this, "Failed to start", "Failed to start Paraview in directoy "+ui->localDir->text());
  }
}


void MainWindow::onStartRemoteParaview()
{
  if (ensureConnected())
  {
    IQRemoteParaviewDialog dlg(remote_->exeConfig(), this);
    dlg.exec();
  }
}


void MainWindow::onClearProgressCharts()
{
  ui->graph->reset();
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}


void MainWindow::saveSettings()
{
    QSettings settings("silentdynamics", "isofExecutionManager");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("vsplitter", ui->v_splitter->saveState());
    settings.setValue("bwlimit", bwLimit_);
}


void MainWindow::readSettings()
{
    QSettings settings("silentdynamics", "isofExecutionManager");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    ui->v_splitter->restoreState(settings.value("vsplitter").toByteArray());
    setBWLimit( settings.value("bwlimit", -1).toInt() );
}


AuxiliaryJob::AuxiliaryJob( insight::JobPtr job )
  : job_(job)
{}


void AuxiliaryJob::run()
{
  auto handleOutputLine=[this](const std::string& line)
  {
    std::cout<<line<<std::endl;
    if (!line.empty())
    {
      Q_EMIT outputLineReceived( QString::fromStdString(line) );
    }
  };

  job_->ios_run_with_interruption(
        handleOutputLine,
        handleOutputLine
  );
  job_->wait();

  Q_EMIT completed(job_->process().exit_code());
}


void MainWindow::remoteWriteAndCopyBack(bool parallel)
{
  if (!ensureConnected()) return;

  std::ostringstream cmd;

  cmd
      << remote_->exeConfig().remoteSourceOFEnvStatement()
      << "cd "<<remote_->location().remoteDir()<<" && "
      << "( isofWaitForWrite.sh "
      << (parallel ? "-p" : "") << " )";


//  insight::SSHCommand sc(remote_->server(), { "bash -lc \""+insight::escapeShellSymbols(cmd.str())+"\"" });
//  auto job=std::make_shared<insight::Job>();
//  job->forkProcess(
//      remote_->exeConfig().server()->launchCommand(
//          cmd.str(),
//          job->redirections()
//          ));

  auto job=std::make_shared<insight::Job>(
      remote_->exeConfig().server()->config().commandAndArgs(cmd.str())
      );

//  insight::Job::forkExternalProcess(
//        job, remote_->exeConfig().server()->launchCommand(
//          cmd.str(),
//          boost::process::std_in < in_,
//          boost::process::std_out > out_,
//          boost::process::std_err > err_
//          ));
  auto *aj = new AuxiliaryJob(job);

  connect( aj, &AuxiliaryJob::outputLineReceived,
           ui->log, &LogViewerWidget::appendLine );
  connect( aj, &AuxiliaryJob::completed, this,
           [this,aj](int rv) {
            if (rv==0)
            {
              this->syncRemoteToLocal(false);
            }
            aj->deleteLater();
           });

  aj->moveToThread(&auxJobThread_);
  auxJobThread_.start();
  QMetaObject::invokeMethod(aj, &AuxiliaryJob::run, Qt::QueuedConnection);
}


