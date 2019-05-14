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

    if (num>0) onStartTail();
  }
}


void MainWindow::onStartTail()
{
  if (tsi_)
  {
    tsi_->startTail( [&](std::string line) { std::cout<<line<<std::endl; emit logReady(QString::fromStdString(line)); } );
  }
}

MainWindow::MainWindow(const boost::filesystem::path& location, QWidget *parent) :
  QMainWindow(parent),
  insight::RemoteExecutionConfig(location, false),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  setWindowIcon(QIcon(":/resources/logo_insight_cae.svg"));
  this->setWindowTitle("InsightCAE Execution Manager");

  ui->log->setMaximumBlockCount(10000);
  ui->log->setCenterOnScroll(true);

  connect(ui->actionSelect_Remote_Directory, &QAction::triggered, this, &MainWindow::onSelectRemoteDir);
  connect(ui->action_syncLocalToRemote, &QAction::triggered, this, &MainWindow::syncLocalToRemote);
  connect(ui->action_syncRemoteToLocal, &QAction::triggered, this, &MainWindow::syncRemoteToLocal);
  connect(ui->actionStart_Paraview, &QAction::triggered, this, &MainWindow::onStartParaview);
  connect(ui->actionStart_Remote_Paraview, &QAction::triggered, this, &MainWindow::onStartRemoteParaview);
  connect(ui->actionStart_Remote_Paraview_in_Subdirectory, &QAction::triggered, this, &MainWindow::onStartRemoteParaviewSubdir);

  connect(this, &MainWindow::logReady, ui->log, &QPlainTextEdit::appendPlainText);

#ifdef HAVE_KF5
  terminal_ = new TerminalWidget(ui->v_splitter);
  terminal_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
  ui->v_splitter->addWidget(terminal_);
  terminal_->initialise();
#endif

  updateGUI();
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
