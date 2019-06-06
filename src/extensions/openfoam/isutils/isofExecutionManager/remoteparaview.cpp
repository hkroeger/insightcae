#include "remoteparaview.h"
#include "ui_remoteparaview.h"

RemoteParaview::RemoteParaview(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::RemoteParaview)
{
  ui->setupUi(this);
}

RemoteParaview::~RemoteParaview()
{
  delete ui;
}
