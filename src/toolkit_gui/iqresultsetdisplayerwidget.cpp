#include "iqresultsetdisplayerwidget.h"
#include "ui_iqresultsetdisplayerwidget.h"

#include "iqaddfilterdialog.h"

#include <QFileDialog>
#include <QCheckBox>

#include "base/cppextensions.h"
#include "rapidxml/rapidxml_print.hpp"

IQResultSetDisplayerWidget::IQResultSetDisplayerWidget(QWidget *parent) :
    QWidget(parent),
    resultsModel_(nullptr),
    filteredResultsModel_(nullptr),
    filterModel_(new IQResultSetFilterModel(this)),
    ui(new Ui::IQResultSetDisplayerWidget)
{
    ui->setupUi(this);

    ui->lvResultElementFilters->setModel(filterModel_);
    insight::connectToCWithContentsDisplay(ui->lvFilteredResultSetToC, ui->wiResultElementContent);

    connect(filterModel_, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
            {
                filteredResultsModel_->resetFilter( filterModel_->filter() );
            }
    );
    connect(filterModel_, &QAbstractItemModel::modelReset, this,
            [this]()
            {
                filteredResultsModel_->resetFilter( filterModel_->filter() );
                ui->btnDeleteFilter->setEnabled( false );
            }
    );

    connect(ui->btnAddFilter, &QPushButton::clicked, this,
            [this]()
            {
                if (resultsModel_->resultSet())
                {
                    IQAddFilterDialog dlg(resultsModel_->resultSet(), this);
                    if (dlg.exec() == QDialog::Accepted)
                    {
                        auto f = filterModel_->filter();
                        auto nf = dlg.filter();
                        f.insert( nf.begin(), nf.end() );
                        filterModel_->resetFilter(f);
                    }
                }
            }
    );


    connect(ui->btnDeleteFilter, &QPushButton::clicked, this,
            [this]()
            {
                auto i = ui->lvResultElementFilters->currentIndex();
                if (i.isValid())
                {
                    filterModel_->removeRow(i.row(), i.parent());
                    filteredResultsModel_->resetFilter(filterModel_->filter());
                }
            }
    );

    connect(ui->btnClearFilter, &QPushButton::clicked, this,
            [this]()
            {
                filterModel_->clear();
            }
    );

    connect(ui->lvResultElementFilters->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex &current, const QModelIndex &previous)
            {
                ui->btnDeleteFilter->setEnabled( current.isValid() );
            }
    );

}

IQResultSetDisplayerWidget::~IQResultSetDisplayerWidget()
{
    delete ui;
}

void IQResultSetDisplayerWidget::clear()
{
    loadResults(nullptr);
}

void IQResultSetDisplayerWidget::loadResults(insight::ResultSetPtr results)
{
    auto oldrm = resultsModel_;
    auto foldrm = filteredResultsModel_;

    if (results)
    {
        resultsModel_ = new insight::IQResultSetModel(results, false, this);
        filteredResultsModel_ = new insight::IQFilteredResultSetModel(this);
        filteredResultsModel_->resetFilter( filterModel_->filter() );
        filteredResultsModel_->setSourceModel(resultsModel_);
        ui->lvFilteredResultSetToC->setModel(filteredResultsModel_);
    }
    else
    {
        resultsModel_=nullptr;
        filteredResultsModel_=nullptr;
    }

    if (foldrm) delete foldrm;
    if (oldrm) delete oldrm;

    ui->lvFilteredResultSetToC->expandAll();
    ui->lvFilteredResultSetToC->resizeColumnToContents(0);
    ui->lvFilteredResultSetToC->resizeColumnToContents(1);
}

bool IQResultSetDisplayerWidget::hasResults() const
{
    return ((resultsModel_!=nullptr) && (resultsModel_->hasResults()));
}


void IQResultSetDisplayerWidget::loadResultSet(const std::string& analysisName)
{
    auto f = QFileDialog::getOpenFileName(
                this, "Load result set", "",
                "InsightCAE Result Set (*.isr)" );
    if (!f.isEmpty())
    {
      auto r = insight::ResultSet::createFromFile(f.toStdString(), analysisName);
      loadResults(r);
    }
}


void IQResultSetDisplayerWidget::saveResultSetAs()
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
        cb->setText("Save filtered result set");
        QGridLayout *fdl = static_cast<QGridLayout*>(fd.layout());
        int last_row=fdl->rowCount(); // id of new row below
        fdl->addWidget(cb, last_row, 0, 1, -1);

        cb->setChecked(true);

        if ( fd.exec() == QDialog::Accepted )
        {
          boost::filesystem::path outf =
                  insight::ensureFileExtension(
                      fd.selectedFiles()[0].toStdString(),
                      ".isr"
                  );

          if (cb->isChecked())
          {
              filteredResultsModel_->filteredResultSet()->saveToFile(outf);
          }
          else
          {
              resultsModel_->resultSet()->saveToFile(outf);
          }
        }
    }
}

void IQResultSetDisplayerWidget::renderReport()
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
        cb->setText("Save filtered result set");
        QGridLayout *fdl = static_cast<QGridLayout*>(fd.layout());
        int last_row=fdl->rowCount(); // id of new row below
        fdl->addWidget(cb, last_row, 0, 1, -1);

        cb->setChecked(true);

        if ( fd.exec() == QDialog::Accepted )
        {
          boost::filesystem::path outf =
                  insight::ensureFileExtension(
                      fd.selectedFiles()[0].toStdString(),
                      ".pdf"
                  );
          if (cb->isChecked())
          {
              filteredResultsModel_->filteredResultSet()->generatePDF(outf);
          }
          else
          {
              resultsModel_->resultSet()->generatePDF(outf);
          }
        }
    }
}


void IQResultSetDisplayerWidget::loadFilter()
{
    auto inf = QFileDialog::getOpenFileName(
                this, "Select result filter file",
                "", "InsightCAE Result Set Filter (*.irf)");

    if (!inf.isEmpty())
    {
        insight::CurrentExceptionContext ex("reading result set filter from file "+inf.toStdString());

        std::string contents;
        insight::readFileIntoString(inf.toStdString(), contents);

        rapidxml::xml_document<> doc;
        doc.parse<0>(&contents[0]);

        rapidxml::xml_node<> *rootnode = doc.first_node("root");
        insight::ResultSetFilter rsf;
        rsf.readFromNode(*rootnode);
        filterModel_->resetFilter(rsf);
    }
}

void IQResultSetDisplayerWidget::saveFilter()
{
    auto outf = QFileDialog::getSaveFileName(this, "Select result filter file",
                                            "", "InsightCAE Result Set Filter (*.irf)");
    if (!outf.isEmpty())
    {
        insight::CurrentExceptionContext ex("writing result set filter into file "+outf.toStdString());

        auto outfn = insight::ensureFileExtension(outf.toStdString(), ".irf");

      //   std::cout<<"Writing parameterset to file "<<file<<std::endl;


        // prepare XML document
        rapidxml::xml_document<> doc;
        rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
        decl->append_attribute(doc.allocate_attribute("version", "1.0"));
        decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
        doc.append_node(decl);
        rapidxml::xml_node<> *rootnode = doc.allocate_node(rapidxml::node_element, "root");
        doc.append_node(rootnode);

        filterModel_->filter().appendToNode(doc, *rootnode);

        std::ofstream f(outfn.string());
        f << doc << std::endl;
    }
}
