#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cstdlib>

#include "base/boost_include.h"

#include "openfoam/solveroutputanalyzer.h"

#include <QtGlobal>
#include <QDebug>
#include <QProcess>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <QTimer>
#include <QStatusBar>

#include "base/qt5_helper.h"

#include "remoteparaview.h"
#include "remotesync.h"




void MainWindow::updateGUI()
{
  setWindowTitle( ui->localDir->text() + " - InsightCAE Execution Manager" );

  if (remote_)
  {
    terminal_->changeDir( ui->localDir->text() );
    auto cmd = QString("ssh ")+remote_->server().c_str()+" -t 'cd '"+remote_->remoteDir().c_str()+"'; bash -l'\n";
    terminal_->sendText(cmd);

    tsi_.reset(new insight::TaskSpoolerInterface(remote_->socket(), remote_->server()));
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
  ui(new Ui::MainWindow)
{
  if ( boost::filesystem::exists( insight::RemoteExecutionConfig::defaultConfigFile(location) ) )
  {
    remote_.reset(new insight::RemoteExecutionConfig(location));
  }

  ui->setupUi(this);

  ui->localDir->setText( boost::filesystem::absolute(location).c_str() );

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


  terminal_ = new QTermWidget( 1, ui->tabWidget );

  QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  terminal_->setTerminalFont(font);

  ui->tabWidget->addTab(terminal_, "&4 - Terminal");


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

      ui->server->setText( QString::fromStdString(dlg.selectedServer()) );
      ui->remoteDir->setText( QString::fromStdString(dlg.selectedRemoteDir().string()) );

      insight::RemoteLocation::writeConfigFile(
            insight::RemoteExecutionConfig::defaultConfigFile("."),
            dlg.selectedServer(),
            dlg.selectedRemoteDir()
            );

      updateGUI();
  }
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
      statusBar()->showMessage("Transfer from remote location to local directory finished");
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
