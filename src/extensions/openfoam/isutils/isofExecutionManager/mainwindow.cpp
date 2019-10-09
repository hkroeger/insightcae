#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "terminal.h"

#include <cstdlib>

#include "base/boost_include.h"

#include <QtGlobal>
#include <QDebug>
#include <QProcess>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <QTimer>
#include <QStatusBar>

#include "base/qt5_helper.h"

#include "ui_remoteparaview.h"
#include "remoteparaview.h"

void MainWindow::updateGUI()
{
  if (isValid())
  {
    bfs_path loc_dir=boost::filesystem::absolute(localDir());
    setWindowTitle(QString(loc_dir.c_str())+" - InsightCAE Execution Manager");
    ui->server->setText(server().c_str());
    ui->localDir->setText(loc_dir.c_str());
    ui->remoteDir->setText(remoteDir().c_str());
#ifdef HAVE_KF5
    terminal_->setDirectory(localDir().c_str());
    terminal_->sendInput(QString("ssh ")+server().c_str()+" -t \"cd '"+remoteDir().c_str()+"'; bash -l\"\n");
#endif
    tsi_.reset(new insight::TaskSpoolerInterface(socket(), server()));
    onRefreshJobList();
  }
}



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

MainWindow::MainWindow(const boost::filesystem::path& location, QWidget *parent) :
  QMainWindow(parent),
  insight::RemoteExecutionConfig(location, false),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  setWindowIcon(QIcon(":/resources/logo_insight_cae.svg"));
  this->setWindowTitle("InsightCAE Execution Manager");

  soa_.reset(new insight::SolverOutputAnalyzer(*ui->graph));

  connect(ui->actionSelect_Remote_Directory, &QAction::triggered, this, &MainWindow::onSelectRemoteDir);
  connect(ui->action_syncLocalToRemote, &QAction::triggered, this, &MainWindow::syncLocalToRemote);
  connect(ui->action_syncRemoteToLocal, &QAction::triggered, this, &MainWindow::syncRemoteToLocal);
  connect(ui->sync_to_remote, &QPushButton::clicked, this, &MainWindow::syncLocalToRemote);
  connect(ui->sync_to_local, &QPushButton::clicked, this, &MainWindow::syncRemoteToLocal);

  connect(ui->actionStart_Paraview, &QAction::triggered, this, &MainWindow::onStartParaview);
  connect(ui->actionStart_Remote_Paraview, &QAction::triggered, this, &MainWindow::onStartRemoteParaview);

  connect(this, &MainWindow::logReady, ui->log, &LogViewerWidget::appendLine);
  connect(this, &MainWindow::logReady, this, &MainWindow::updateOutputAnalzer ); // through signal/slot to execute analysis in GUI thread

#ifdef HAVE_KF5
  terminal_ = new TerminalWidget(/*ui->v_splitter*/ ui->tabWidget );
  terminal_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
  ui->tabWidget->addTab(terminal_, "&4 - Terminal");
  //ui->v_splitter->addWidget(terminal_);
  terminal_->initialise();
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

  updateGUI();

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
  delete ui;
}

void MainWindow::onSelectRemoteDir()
{
  RemoteDirSelector dlg(this);
  if (dlg.exec() == QDialog::Accepted)
  {
      server_=dlg.selectedServer();
      remoteDir_=dlg.selectedRemoteDir();

      std::ofstream cfg("meta.foam");
      cfg << server_ << ":" << remoteDir_.string();

      updateGUI();
  }
}

RunSyncToRemote::RunSyncToRemote(insight::RemoteExecutionConfig& rec)
  : rec_(rec)
{}

void RunSyncToRemote::run()
{
  rec_.syncToRemote
      (
        {},
        [&](int progress, const std::string& progress_text)
        {
          emit progressValueChanged(progress);
          emit progressTextChanged(QString(progress_text.c_str()));
        }
      );
  emit transferFinished();
}

void MainWindow::syncLocalToRemote()
{
  RunSyncToRemote *rstr = new RunSyncToRemote(*this);
  connect(rstr, &RunSyncToRemote::progressValueChanged, progressbar_, &QProgressBar::setValue);
  connect(rstr, &RunSyncToRemote::progressTextChanged,
          this, [=](const QString& text) { statusBar()->showMessage(text); } );
  connect(rstr, &RunSyncToRemote::transferFinished,
          this, [&]()
  {
    progressbar_->setHidden(true);
    statusBar()->showMessage("Transfer to remote location finished");
  });
  connect(rstr, &RunSyncToRemote::transferFinished, rstr, &QObject::deleteLater);

  progressbar_->setHidden(false);
  statusBar()->showMessage("Transfer to remote location started");

  rstr->start();
}



RunSyncToLocal::RunSyncToLocal(insight::RemoteExecutionConfig& rec)
  : rec_(rec)
{}

void RunSyncToLocal::run()
{
  rec_.syncToLocal
      (
        false,
        {},
        [&](int progress, const std::string& progress_text)
        {
          emit progressValueChanged(progress);
          emit progressTextChanged(QString(progress_text.c_str()));
        }
      );
  emit transferFinished();
}

void MainWindow::syncRemoteToLocal()
{
    RunSyncToLocal* rstl = new RunSyncToLocal(*this);
    connect(rstl, &RunSyncToLocal::progressValueChanged,
            progressbar_, &QProgressBar::setValue);
    connect(rstl, &RunSyncToLocal::progressTextChanged,
            this, [=](const QString& text) { statusBar()->showMessage(text); } );
    connect(rstl, &RunSyncToLocal::transferFinished, rstl, &QObject::deleteLater);
    connect(rstl, &RunSyncToLocal::transferFinished,
            this, [&]()
    {
      progressbar_->setHidden(true);
      statusBar()->showMessage("Transfer from remote location to local directory finished");
    });

    progressbar_->setHidden(false);
    statusBar()->showMessage("Transfer from remote location to local directory started");

    rstl->start();
}

void MainWindow::onStartParaview()
{
  if (!QProcess::startDetached("isPV.py", QStringList(), localDir().c_str() ))
  {
    QMessageBox::critical(this, "Failed to start", QString("Failed to start Paraview in directoy ")+localDir().c_str());
  }
}


void MainWindow::onStartRemoteParaview()
{
  RemoteParaview dlg(this);
//  dlg.ui->subdir->setText(rem_subdir_);

  auto rds=remoteSubdirs();
  for (auto d: rds) dlg.ui->subdir->addItem( QString::fromStdString(d.string()) );

  dlg.ui->remhost->setText(rem_host_);
  if (dlg.exec())
  {
    QStringList args;

//    rem_subdir_ = dlg.ui->subdir->text();
    rem_subdir_ = dlg.ui->subdir->currentText();

    rem_host_ = dlg.ui->remhost->text();

//    if (!dlg.ui->subdir->text().isEmpty())
    if (!rem_subdir_.isEmpty())
      args << "-s" << rem_subdir_ /*dlg.ui->subdir->text()*/;

    if (!dlg.ui->remhost->text().isEmpty())
      args << "-r" << dlg.ui->remhost->text();

    if (!QProcess::startDetached("isPVRemote.sh", args, localDir().c_str() ))
    {
      QMessageBox::critical(this, "Failed to start", QString("Failed to start Paraview in directoy ")+localDir().c_str());
    }
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
    settings.setValue("remhost", rem_host_);
    settings.setValue("remsubdir", rem_subdir_);
    settings.setValue("vsplitter", ui->v_splitter->saveState());
}

void MainWindow::readSettings()
{
    QSettings settings("silentdynamics", "isofExecutionManager");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    rem_host_=settings.value("remhost").toString();
    rem_subdir_=settings.value("remsubdir").toString();
    ui->v_splitter->restoreState(settings.value("vsplitter").toByteArray());
}
