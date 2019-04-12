
#include <QProcess>
#include <QDebug>

#include "taskspoolermonitor.h"
#include "ui_taskspoolermonitor.h"

#include "base/qt5_helper.h"

TaskSpoolerMonitor::TaskSpoolerMonitor(const boost::filesystem::path& tsp_socket, QWidget *parent) :
  QWidget(parent),
  tsp_socket_(tsp_socket),
  env_( QProcessEnvironment::systemEnvironment() ),
  ui(new Ui::TaskSpoolerMonitor)
{
  ui->setupUi(this);

  env_.insert("TS_SOCKET", tsp_socket_.c_str());

  connect(ui->btn_refresh, &QPushButton::clicked, this, &TaskSpoolerMonitor::onRefresh);
  connect(ui->btn_kill, &QPushButton::clicked, this, &TaskSpoolerMonitor::onKill);
  connect(ui->btn_clean, &QPushButton::clicked, this, &TaskSpoolerMonitor::onClean);

  onRefresh();
}

TaskSpoolerMonitor::~TaskSpoolerMonitor()
{
  delete ui;
}

void TaskSpoolerMonitor::onRefresh()
{
  QProcess process;
  process.setProcessEnvironment(env_);
  process.start( "tsp" );
  process.waitForFinished();
  QStringList lines = QString( process.readAllStandardOutput() ).split('\n');

  ui->joblist->clear();
  int num=0;
  for (int i=1; i<lines.size(); i++)
  {
    //QRegExp re("^([^ ]+) *([^ ]+) *(.*)$");
    //if ( (pos = re.indexIn(lines[i], pos)) != -1 )

    QStringList tks = lines[i].split(' ', QString::SkipEmptyParts);
    if (tks.size()>2)
    {
      if (tks[1]!="finished")
      {
        ui->joblist->addItem(lines[i]);
        num++;
      }
    }
  }

  if (num>0) onStartTail();
}


void TaskSpoolerMonitor::onClean()
{
  QProcess process;
  process.setProcessEnvironment(env_);
  process.start( "tsp -C" );
  process.waitForFinished();

  onRefresh();
}


void TaskSpoolerMonitor::onKill()
{
  QProcess process;
  process.setProcessEnvironment(env_);
  process.start( "tsp -k" );
  process.waitForFinished();

  onRefresh();
}


void TaskSpoolerMonitor::onStartTail()
{
  QProcess *p=new QProcess(this);
  p->setProcessEnvironment(env_);
  connect(p, &QProcess::readyReadStandardOutput, this, &TaskSpoolerMonitor::onOutputReady);
  connect(p, &QProcess::readyReadStandardError, this, &TaskSpoolerMonitor::onErrorReady);
  connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this, &TaskSpoolerMonitor::onFinishedTail);
  p->start( "tsp", { "-t" } );
}

void TaskSpoolerMonitor::onFinishedTail(int , QProcess::ExitStatus )
{
  onClean();
}

void TaskSpoolerMonitor::onOutputReady()
{
  QProcess *p = dynamic_cast<QProcess *>( sender() );
  if (p)
  {
    ui->log->appendPlainText( p->readAllStandardOutput() );
  }
}

void TaskSpoolerMonitor::onErrorReady()
{
  QProcess *p = dynamic_cast<QProcess *>( sender() );
  if (p)
  {
    ui->log->appendPlainText( p->readAllStandardError() );
  }
}

TaskSpoolerMonitorDialog::TaskSpoolerMonitorDialog(const boost::filesystem::path& tsp_socket, QWidget *parent)
  : QDialog(parent)
{
  QVBoxLayout *l=new QVBoxLayout;
  setLayout(l);
  auto* w = new TaskSpoolerMonitor(tsp_socket, this);
  l->addWidget(w);
//  QLabel *status=new QLabel(this);
//  l->addWidget(status);
//  connect(ofcc, &OFCleanCaseForm::statusMessage,
//          status, &QLabel::setText);
}
