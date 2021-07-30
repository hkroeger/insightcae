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

#include "remoteparaview.h"
#include "remotesync.h"
#include "base/sshlinuxserver.h"


void IQRXRemoteExecutionState::updateGUI(bool enabled)
{
  auto *mw = dynamic_cast<MainWindow*>(parent());
  auto *ui = mw->ui;

  if (enabled)
  {
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
   qDebug() << fi.family() << fi.fixedPitch();
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

MainWindow::MainWindow(const boost::filesystem::path& location, QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  auto reccfg = insight::RemoteExecutionConfig::defaultConfigFile(location);
  if ( boost::filesystem::exists( reccfg ) )
  {
    //remote_.reset(new insight::RemoteExecutionConfig(location));
    remote_ = IQRemoteExecutionState::New<IQRXRemoteExecutionState>(
          this, reccfg );
  }

  ui->setupUi(this);

  ui->localDir->setText( QString::fromStdString(boost::filesystem::absolute(location).string()) );

  setWindowIcon(QIcon(":/resources/logo_insight_cae.svg"));
  this->setWindowTitle("InsightCAE Execution Manager");

  soa_.reset(new insight::SolverOutputAnalyzer(*ui->graph));

  connect(ui->actionSelect_remote_server, &QAction::triggered, this,
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

  connect(ui->actionSelect_Remote_Directory, &QAction::triggered, this,
          [&]()
          {
            RemoteDirSelector dlg(this);

            if (dlg.exec() == QDialog::Accepted)
            {

                ui->server->setText( QString::fromStdString(dlg.selectedServer()) );
                ui->remoteDir->setText( QString::fromStdString(dlg.selectedRemoteDir().string()) );

                remote_ = IQRXRemoteExecutionState::New(
                      insight::remoteServers.findServer(
                        dlg.selectedServer() ),
                      dlg.selectedRemoteDir()
                      ).writeConfigFile(
                      insight::RemoteExecutionConfig::defaultConfigFile("."));
            }
          }
  );

  connect(ui->action_syncLocalToRemote, &QAction::triggered, this, &MainWindow::syncLocalToRemote);
  connect(ui->action_syncRemoteToLocal, &QAction::triggered, this, &MainWindow::syncRemoteToLocal);
  connect(ui->sync_to_remote, &QPushButton::clicked, this, &MainWindow::syncLocalToRemote);
  connect(ui->sync_to_local, &QPushButton::clicked, this, &MainWindow::syncRemoteToLocal);
  connect(ui->actionRemote_write_and_copy_to_local, &QAction::triggered,
          [=]() {
            this->remoteWriteAndCopyBack(false);
          });
  connect(ui->actionRemote_write_reconstruct_and_copy_to_local, &QAction::triggered,
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
  connect(ui->btn_kill, &QPushButton::clicked,
          [&]() {
            if (tsi_) {
              tsi_->kill();
              tsi_->stopTail();
              onRefreshJobList();
            }
          }
  );

  connect(ui->btn_clean, &QPushButton::clicked,
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

  readSettings(); 

  progressbar_=new QProgressBar(this);
  statusBar()->addPermanentWidget(progressbar_);
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





void MainWindow::syncLocalToRemote()
{
  if (remote_)
  {
    auto *rstr = new insight::RunSyncToRemote(*remote_);

    connect(rstr, &insight::RunSyncToRemote::progressValueChanged, progressbar_, &QProgressBar::setValue);
    connect(rstr, &insight::RunSyncToRemote::progressTextChanged,
            this, [=](const QString& text) { statusBar()->showMessage(text); } );
    connect(rstr, &insight::RunSyncToRemote::transferFinished,
            this, [&]()
    {
      progressbar_->setHidden(true);
      statusBar()->showMessage("Transfer to remote location finished");
    });
    connect(rstr, &insight::RunSyncToRemote::transferFinished, rstr, &QObject::deleteLater);

    progressbar_->setHidden(false);
    statusBar()->showMessage("Transfer to remote location started");

    rstr->start();
  }
}





void MainWindow::syncRemoteToLocal()
{
  if (remote_)
  {
    auto* rstl = new insight::RunSyncToLocal(*remote_);

    connect(rstl, &insight::RunSyncToLocal::progressValueChanged,
            progressbar_, &QProgressBar::setValue);
    connect(rstl, &insight::RunSyncToLocal::progressTextChanged,
            this, [=](const QString& text) { statusBar()->showMessage(text); } );
    connect(rstl, &insight::RunSyncToLocal::transferFinished, rstl, &QObject::deleteLater);
    connect(rstl, &insight::RunSyncToLocal::transferFinished,
            this, [&]()
    {
      progressbar_->setHidden(true);
      statusBar()->showMessage(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss.z : ")+"Transfer from remote location to local directory finished");
    });

    progressbar_->setHidden(false);
    statusBar()->showMessage("Transfer from remote location to local directory started");

    rstl->start();
  }
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
    RemoteParaview dlg(*remote_, this);
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
}


void MainWindow::readSettings()
{
    QSettings settings("silentdynamics", "isofExecutionManager");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    ui->v_splitter->restoreState(settings.value("vsplitter").toByteArray());
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
  job_->process->wait();

  Q_EMIT completed(job_->process->exit_code());
}


void MainWindow::remoteWriteAndCopyBack(bool parallel)
{
  std::ostringstream cmd;

  cmd
      << remote_->remoteSourceOFEnvStatement()
      << "cd "<<remote_->remoteDir()<<" && "
      << "( isofWaitForWrite.sh "
      << (parallel ? "-p" : "") << " )";


  auto job=std::make_shared<insight::Job>();
//  insight::SSHCommand sc(remote_->server(), { "bash -lc \""+insight::escapeShellSymbols(cmd.str())+"\"" });
  insight::forkExternalProcess(
        job, remote_->server()->launchCommand(
          cmd.str(),
          boost::process::std_in < job->in,
          boost::process::std_out > job->out,
          boost::process::std_err > job->err
          ));
  auto *aj = new AuxiliaryJob(job);

  connect( aj, &AuxiliaryJob::outputLineReceived,
           ui->log, &LogViewerWidget::appendLine );
  connect( aj, &AuxiliaryJob::completed,
           [this,aj](int rv) {
            if (rv==0)
            {
              this->syncRemoteToLocal();
            }
            aj->deleteLater();
           });

  aj->moveToThread(&auxJobThread_);
  auxJobThread_.start();
  QMetaObject::invokeMethod(aj, &AuxiliaryJob::run, Qt::QueuedConnection);
}


