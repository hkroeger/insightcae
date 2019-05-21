#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "terminal.h"

#include <cstdlib>

#include "base/boost_include.h"

#include "remotedirselector.h"

#include <QDebug>
#include <QProcess>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>

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

void MainWindow::onRefreshJobList()
{
  ui->commands->clear();

  if (tsi_)
  {
    auto jl = tsi_->jobs();


    int num=0;
    for (auto j: jl)
    {
      if (j.state!=insight::TaskSpoolerInterface::Finished)
      {
        ui->commands->addItem( QString::number(j.id) +" "+ QString::fromStdString(j.remainder) );
        num++;
      }
    }

    if ( (num>0) && !tsi_->isTailRunning() )
    {
      onStartTail();
    }
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
  connect(ui->actionStart_Paraview, &QAction::triggered, this, &MainWindow::onStartParaview);
  connect(ui->actionStart_Remote_Paraview, &QAction::triggered, this, &MainWindow::onStartRemoteParaview);
  connect(ui->actionStart_Remote_Paraview_in_Subdirectory, &QAction::triggered, this, &MainWindow::onStartRemoteParaviewSubdir);

  connect(this, &MainWindow::logReady, ui->log, &LogViewerWidget::appendLine);
  connect(this, &MainWindow::logReady, this, &MainWindow::updateOutputAnalzer ); // through signal/slot to execute analysis in GUI thread

#ifdef HAVE_KF5
  terminal_ = new TerminalWidget(ui->v_splitter);
  terminal_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
  ui->v_splitter->addWidget(terminal_);
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

  updateGUI();

  readSettings();
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

void MainWindow::syncLocalToRemote()
{
    syncToRemote();
}

void MainWindow::syncRemoteToLocal()
{
    syncToLocal();
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
  if (!QProcess::startDetached("isPVRemote.sh", QStringList(), localDir().c_str() ))
  {
    QMessageBox::critical(this, "Failed to start", QString("Failed to start Paraview in directoy ")+localDir().c_str());
  }
}



void MainWindow::onStartRemoteParaviewSubdir()
{
  QString sd = QInputDialog::getText(this, "Enter subdirectory name", "Please enter the name of the remote subdirectory");
  if (!sd.isEmpty())
  {
    if (!QProcess::startDetached("isPVRemote.sh", QStringList() << "-s" << sd, localDir().c_str() ))
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
    settings.setValue("vsplitter", ui->v_splitter->saveState());
}

void MainWindow::readSettings()
{
    QSettings settings("silentdynamics", "isofExecutionManager");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    ui->v_splitter->restoreState(settings.value("vsplitter").toByteArray());
}
