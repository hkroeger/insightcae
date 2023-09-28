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

#include "base/qt5_helper.h"

#include "iqremoteparaviewdialog.h"
#include "remotesync.h"
#include "base/sshlinuxserver.h"


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
              .arg(QString::fromStdString(ssh->serverConfig()->hostName_))
              .arg(QString::fromStdString(rlc_->remoteDir().string()))
              ;
          mw->terminal_->sendText(cmd);
        }
#endif

          mw->tsi_.reset(new insight::TaskSpoolerInterface(
                       exeConfig().socket(), exeConfig().server()));
          mw->onRefreshJobList();
        }
      }
      else
      {
        mw->setWindowTitle( ui->localDir->text() + " - InsightCAE Execution Manager" );
        mw->tsi_.reset();
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


  setWindowIcon(QIcon(":/resources/logo_insight_cae.svg"));
  this->setWindowTitle("InsightCAE Execution Manager");

  soa_.reset(new insight::SolverOutputAnalyzer(*ui->graph));

  connect(
      ui->actionSelect_remote_server, &QAction::triggered, this,
      [&]()
      {
        QStringList servers;
        for (const auto& s: insight::remoteServers)
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

        if (!selectedServer.isEmpty())
        {
          remote_ = IQRemoteExecutionState::New<IQRXRemoteExecutionState>(
                this,
                insight::remoteServers.findServer(
                  selectedServer.toStdString() )
                );
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
              auto server = remote_->location().serverConfig()->getInstanceIfRunning();
              insight::assertion(bool(server), "server is not running");

              RemoteDirSelector dlg(this, server);

              if (dlg.exec() == QDialog::Accepted)
              {

                  ui->remoteDir->setText( QString::fromStdString(dlg.selectedRemoteDir().string()) );

                  remote_ = IQRemoteExecutionState::New<IQRXRemoteExecutionState>(
                      this,
                      remote_->location().serverConfig(),
                      dlg.selectedRemoteDir()
                      );
                  remote_->location().writeConfigFile(
                      insight::RemoteExecutionConfig::defaultConfigFile("."));
              }
          }
      }
  );

  connect(ui->action_syncLocalToRemote, &QAction::triggered, this,
          std::bind(&MainWindow::syncLocalToRemote, this, false) );
  connect(ui->action_syncRemoteToLocal, &QAction::triggered, this,
          std::bind(&MainWindow::syncRemoteToLocal, this, false, -1) );
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
          std::bind(&MainWindow::syncLocalToRemote, this, true) );
  connect(ui->action_syncRemoteToLocal_includeprocessor, &QAction::triggered, this,
          std::bind(&MainWindow::syncRemoteToLocal, this, true, -1) );
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

  connect(ui->sync_to_remote, &QPushButton::clicked, this, &MainWindow::syncLocalToRemote);
  connect(ui->sync_to_local, &QPushButton::clicked, this,
          std::bind(&MainWindow::syncRemoteToLocal, this, false, -1) );
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
    remote_ = IQRemoteExecutionState::New<IQRXRemoteExecutionState>(
          this, reccfg );
    remote_->commit(location);
  }

  readSettings(); // after remote location is set

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





void MainWindow::syncLocalToRemote(bool includeProcDirs)
{
  if (remote_)
  {
    auto *rstr = new insight::RunSyncToRemote(
                remote_->exeConfig(),
                includeProcDirs );

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
            }
    );
    connect(rstr, &insight::RunSyncToRemote::transferFinished, rstr, &QObject::deleteLater);

    auto stopBtn = new QPushButton("STOP");
    statusBar()->addPermanentWidget(stopBtn);
    connect(stopBtn, &QPushButton::clicked, rstr,
            std::bind(&insight::RunSyncToLocal::interrupt, rstr) );
    connect(rstr, &insight::RunSyncToRemote::transferFinished,
            stopBtn, &QObject::deleteLater);

    statusBar()->showMessage("Transfer to remote location started");

    rstr->start();
  }
}





void MainWindow::syncRemoteToLocal(bool includeProcDirs, int autoRestartSeconds)
{
  if (remote_)
  {
    auto* rstl = new insight::RunSyncToLocal(
                remote_->exeConfig(),
                includeProcDirs);

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

    statusBar()->showMessage("Transfer from remote location to local directory started");

    if (autoRestartSeconds>0)
    {

      connect(rstl, &insight::RunSyncToLocal::transferFinished, rstl,
          [this,includeProcDirs,autoRestartSeconds]()
          {
            auto timer = new QTimer(this);
            timer->setSingleShot(true);
            connect(timer, &QTimer::timeout, this,
              std::bind(&MainWindow::syncRemoteToLocal, this, includeProcDirs, autoRestartSeconds)
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

    if (remote_)
    {
        auto srv = remote_->exeConfig().server();
        srv->setTransferBandWidthLimit(bwlimit);

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
  if (remote_)
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
    if (remote_)
    {
        settings.setValue(
                    "bwlimit",
                    remote_->exeConfig().server()
                    ->transferBandWidthLimit() );
    }
}


void MainWindow::readSettings()
{
    QSettings settings("silentdynamics", "isofExecutionManager");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    ui->v_splitter->restoreState(settings.value("vsplitter").toByteArray());
    if (remote_)
    {
        remote_->exeConfig().server()
                ->setTransferBandWidthLimit(
                    settings.value("bwlimit").toInt()
                    );
    }
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
      remote_->exeConfig().server()->commandAndArgs(cmd.str())
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


