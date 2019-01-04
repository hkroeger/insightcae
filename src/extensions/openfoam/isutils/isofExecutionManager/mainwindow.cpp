#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cstdlib>

#include "base/boost_include.h"

#include "remotedirselector.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  connect(ui->actionSelect_Remote_Directory, &QAction::triggered, this, &MainWindow::onSelectRemoteDir);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::onSelectRemoteDir()
{

//  std::system("sshfs
  RemoteDirSelector dlg(this);
  if (dlg.exec() == QDialog::Accepted)
  {
      std::cout<<dlg.selectedServer()<<":"<<dlg.selectedRemoteDir().string();
  }
}

