#include "graphprogressdisplayertestwindow.h"
#include "ui_graphprogressdisplayertestwindow.h"

#include <QTimer>

GraphProgressDisplayerTestWindow::GraphProgressDisplayerTestWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::GraphProgressDisplayerTestWindow)
{
  ui->setupUi(this);

  auto t = new QTimer;
  connect(t, &QTimer::timeout,
          [&]()
          {
            insight::ProgressVariableList pvl;
            pvl["test1/var"]=(double)rand() / RAND_MAX;
            pvl["residual/Ux"]=(double)rand() / RAND_MAX;
            pvl["residual/p"]=0.1*(double)rand() / RAND_MAX;
            ui->graph->update(insight::ProgressState(++iter_, pvl));
          }
  );
  t->setInterval(10);
  t->start();
}

GraphProgressDisplayerTestWindow::~GraphProgressDisplayerTestWindow()
{
  delete ui;
}
