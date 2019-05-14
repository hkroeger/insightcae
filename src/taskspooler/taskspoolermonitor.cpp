
#include <QProcess>
#include <QDebug>

#include "taskspoolermonitor.h"
#include "ui_taskspoolermonitor.h"

#include "base/qt5_helper.h"

TaskSpoolerMonitor::TaskSpoolerMonitor(const boost::filesystem::path& tsp_socket, const QString& remote_machine, QWidget *parent) :
  QWidget(parent),
  insight::TaskSpoolerInterface (tsp_socket, remote_machine.toStdString()),
  ui(new Ui::TaskSpoolerMonitor)
{
  ui->setupUi(this);

  ui->log->setMaximumBlockCount(10000);
  ui->log->setCenterOnScroll(true);

//  env_.insert("TS_SOCKET", tsp_socket_.c_str());

  connect(ui->btn_refresh, &QPushButton::clicked, this, &TaskSpoolerMonitor::onRefresh);
  connect(ui->btn_kill, &QPushButton::clicked, this, &TaskSpoolerMonitor::onKill);
  connect(ui->btn_clean, &QPushButton::clicked, this, &TaskSpoolerMonitor::onClean);

  connect(this, &TaskSpoolerMonitor::outputReady, ui->log, &QPlainTextEdit::appendPlainText);

  onRefresh();
}

TaskSpoolerMonitor::~TaskSpoolerMonitor()
{
  delete ui;
}

void TaskSpoolerMonitor::onRefresh()
{
  auto jl = jobs();

  ui->joblist->clear();

  int num=0;
  for (auto j: jl)
  {
    if (j.state!=Finished)
    {
      ui->joblist->addItem( QString::number(j.id) +" "+ QString::fromStdString(j.remainder) );
      num++;
    }
  }

  if (num>0) onStartTail();
}


void TaskSpoolerMonitor::onClean()
{
  clean();

  onRefresh();
}


void TaskSpoolerMonitor::onKill()
{
  kill();

  onRefresh();
}


void TaskSpoolerMonitor::onStartTail()
{
//  QProcess *p=new QProcess(this);
//  p->setProcessEnvironment(env_);
//  connect(p, &QProcess::readyReadStandardOutput, this, &TaskSpoolerMonitor::onOutputReady);
//  connect(p, &QProcess::readyReadStandardError, this, &TaskSpoolerMonitor::onErrorReady);
//  connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
//          this, &TaskSpoolerMonitor::onFinishedTail);
//  p->start( "tsp", { "-t" } );
  startTail( [&](std::string line) { emit outputReady(QString::fromStdString(line)); } );
}

void TaskSpoolerMonitor::onFinishedTail(int , QProcess::ExitStatus )
{
  onClean();
}

//void TaskSpoolerMonitor::onOutputReady(const std::string& line)
//{
////  QProcess *p = dynamic_cast<QProcess *>( sender() );
////  if (p)
////  {
////    ui->log->appendPlainText( p->readAllStandardOutput() );
////  }
//  ui->log->appendPlainText( QString::fromStdString(line) );
//}

//void TaskSpoolerMonitor::onErrorReady()
//{
////  QProcess *p = dynamic_cast<QProcess *>( sender() );
////  if (p)
////  {
////    ui->log->appendPlainText( p->readAllStandardError() );
////  }
//}

TaskSpoolerMonitorDialog::TaskSpoolerMonitorDialog(const boost::filesystem::path& tsp_socket, const QString& remote_machine, QWidget *parent)
  : QDialog(parent)
{
  QVBoxLayout *l=new QVBoxLayout;
  setLayout(l);
  auto* w = new TaskSpoolerMonitor(tsp_socket, remote_machine, this);
  l->addWidget(w);
//  QLabel *status=new QLabel(this);
//  l->addWidget(status);
//  connect(ofcc, &OFCleanCaseForm::statusMessage,
//          status, &QLabel::setText);
}
