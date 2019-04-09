#include "taskspoolermonitor.h"
#include "ui_taskspoolermonitor.h"

TaskSpoolerMonitor::TaskSpoolerMonitor(const boost::filesystem::path& tsp_socket, QWidget *parent) :
  QWidget(parent),
  tsp_socket_(tsp_socket),
  ui(new Ui::TaskSpoolerMonitor)
{
  ui->setupUi(this);
}

TaskSpoolerMonitor::~TaskSpoolerMonitor()
{
  delete ui;
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
