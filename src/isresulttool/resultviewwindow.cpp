#include "resultviewwindow.h"
#include "ui_resultviewwindow.h"

#include <QFileDialog>
#include <QCheckBox>


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


  connect(ui->actionSaveAs, &QAction::triggered, this,
          [&]()
  {
      if (resultsModel_)
      {

          QFileDialog fd(this);
          fd.setOption(QFileDialog::DontUseNativeDialog, true);
          fd.setWindowTitle("Save Result Set");
          QStringList filters;
          filters << "InsightCAE Result Set (*.isr)";
          fd.setNameFilters(filters);

          QCheckBox* cb = new QCheckBox;
          cb->setText("Only include the selected result elements");
          QGridLayout *fdl = static_cast<QGridLayout*>(fd.layout());
          int last_row=fdl->rowCount(); // id of new row below
          fdl->addWidget(cb, last_row, 0, 1, -1);

          cb->setChecked(false);

          if ( fd.exec() == QDialog::Accepted )
          {
            boost::filesystem::path outf =
                    insight::ensureFileExtension(
                        fd.selectedFiles()[0].toStdString(),
                        ".isr"
                    );

            if (cb->isChecked())
            {
                resultsModel_->filteredResultSet()->saveToFile(outf);
            }
            else
            {
                resultsModel_->resultSet()->saveToFile(outf);
            }
          }
      }
  }
  );

  connect(ui->actionRender, &QAction::triggered, this,
          [&]()
  {
      if (resultsModel_)
      {

          QFileDialog fd(this);
          fd.setOption(QFileDialog::DontUseNativeDialog, true);
          fd.setWindowTitle("Render Report");
          QStringList filters;
          filters << "PDF document (*.pdf)";
          fd.setNameFilters(filters);

          QCheckBox* cb = new QCheckBox;
          cb->setText("Only include the selected result elements");
          QGridLayout *fdl = static_cast<QGridLayout*>(fd.layout());
          int last_row=fdl->rowCount(); // id of new row below
          fdl->addWidget(cb, last_row, 0, 1, -1);

          cb->setChecked(false);

          if ( fd.exec() == QDialog::Accepted )
          {
            boost::filesystem::path outf =
                    insight::ensureFileExtension(
                        fd.selectedFiles()[0].toStdString(),
                        ".pdf"
                    );
            if (cb->isChecked())
            {
                resultsModel_->filteredResultSet()->generatePDF(outf);
            }
            else
            {
                resultsModel_->resultSet()->generatePDF(outf);
            }
          }
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
