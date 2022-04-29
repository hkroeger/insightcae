#include "resultviewwindow.h"
#include "ui_resultviewwindow.h"
#include "iqaddfilterdialog.h"

#include <QFileDialog>
#include <QCheckBox>
#include <QDebug>

#include "base/cppextensions.h"

#include "rapidxml/rapidxml_print.hpp"


ResultViewWindow::ResultViewWindow(QWidget *parent) :
  QMainWindow(parent),
  viewer_(new IQResultSetDisplayerWidget(this)),
  ui(new Ui::ResultViewWindow)
{
  ui->setupUi(this);

  setWindowTitle("InsightCAE Result Set Viewer");
  setWindowIcon(QIcon(":/logo_insight_cae.png"));

  setCentralWidget(viewer_);


  connect(ui->actionLoad, &QAction::triggered, viewer_,
          [this]() { viewer_->loadResultSet(); } );

  connect(ui->actionSaveAs, &QAction::triggered,
          viewer_, &IQResultSetDisplayerWidget::saveResultSetAs );

  connect(ui->actionRender, &QAction::triggered,
          viewer_, &IQResultSetDisplayerWidget::renderReport );

  connect(ui->actionLoad_filter, &QAction::triggered,
          viewer_, &IQResultSetDisplayerWidget::loadFilter );

  connect(ui->actionSave_filter, &QAction::triggered,
          viewer_, &IQResultSetDisplayerWidget::saveFilter );
}




ResultViewWindow::~ResultViewWindow()
{
    delete ui;
}




void ResultViewWindow::loadResults(insight::ResultSetPtr results)
{
    viewer_->loadResults(results);
}
