#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cstdlib>

#include "base/boost_include.h"

#include "remotedirselector.h"

#include <QDebug>

void MainWindow::updateGUI()
{
    if (isValid())
    {
        ui->server->setText(server_.c_str());
        ui->remoteDir->setText(remoteDir_.c_str());
    }
}

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  insight::RemoteExecutionConfig(".", false),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  connect(ui->actionSelect_Remote_Directory, &QAction::triggered, this, &MainWindow::onSelectRemoteDir);
  connect(ui->action_syncLocalToRemote, &QAction::triggered, this, &MainWindow::syncLocalToRemote);
  connect(ui->action_syncRemoteToLocal, &QAction::triggered, this, &MainWindow::syncRemoteToLocal);

  updateGUI();
}

MainWindow::~MainWindow()
{
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
