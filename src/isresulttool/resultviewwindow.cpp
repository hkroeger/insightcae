#include "resultviewwindow.h"
#include "ui_resultviewwindow.h"

#include <QFileDialog>



ResultViewWindow::ResultViewWindow(QWidget *parent) :
  QMainWindow(parent),
  resultsModel_(nullptr),
  ui(new Ui::ResultViewWindow)
{
  ui->setupUi(this);

  setWindowTitle("InsightCAE Result Set Viewer");
  setWindowIcon(QIcon(":/logo_insight_cae.png"));

  insight::connectToCWithContentsDisplay(ui->toc, ui->content);

  connect(ui->actionLoad, &QAction::triggered, this,
          [&]()
  {
      auto f = QFileDialog::getOpenFileName(this, "Load result set", "", "InsightCAE Result Set (*.isr)");
      if (!f.isEmpty())
      {
        auto r = insight::ResultSet::createFromFile(f.toStdString());
        loadResults(r);
      }
  }
  );

  connect(ui->actionRender, &QAction::triggered, this,
          [&]()
  {
      if (resultsModel_)
      {
          auto rf = resultsModel_->filteredResultSet();
          auto outf = QFileDialog::getSaveFileName(this, "Render Report", "", "PDF document (*.pdf)");
          if (!outf.isEmpty())
            rf->generatePDF(outf.toStdString());
      }
  }
  );
}

ResultViewWindow::~ResultViewWindow()
{
    delete ui;
}

void ResultViewWindow::loadResults(insight::ResultSetPtr results)
{
    auto oldrm=resultsModel_;
    resultsModel_ = new insight::IQResultSetModel(results, true, this);
    ui->toc->setModel(resultsModel_);
    if (oldrm) delete oldrm;

    ui->toc->expandAll();
    ui->toc->resizeColumnToContents(0);
    ui->toc->resizeColumnToContents(1);
}
