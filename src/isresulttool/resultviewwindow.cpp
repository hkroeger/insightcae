#include "resultviewwindow.h"
#include "ui_resultviewwindow.h"


ResultViewWindow::ResultViewWindow(insight::ResultSetPtr results, QWidget *parent) :
  QDialog(parent),
  resultsModel_(results),
  ui(new Ui::ResultViewWindow)
{
  ui->setupUi(this);

  setWindowTitle("InsightCAE Result Set Viewer");
  setWindowIcon(QIcon(":/logo_insight_cae.png"));

  insight::connectToCWithContentsDisplay(ui->toc, ui->content);

  ui->toc->setModel(&resultsModel_);
  ui->toc->expandAll();
  ui->toc->resizeColumnToContents(0);
  ui->toc->resizeColumnToContents(1);
}

ResultViewWindow::~ResultViewWindow()
{
  delete ui;
}
