#include "iqmanagereporttemplatesdialog.h"
#include "ui_iqmanagereporttemplatesdialog.h"

#include <QFileDialog>
#include <QMessageBox>

IQManageReportTemplatesDialog::IQManageReportTemplatesDialog(QWidget *parent) :
    QDialog(parent),
    rtm_(this),
    ui(new Ui::IQManageReportTemplatesDialog)
{
    ui->setupUi(this);
    ui->lvTemplates->setModel(&rtm_);
    ui->lvTemplates->setAlternatingRowColors(true);

    for (const auto&t: insight::ResultReportTemplates::globalInstance())
    {
        std::cout<<"TEMPLATE "<<t.first<<std::endl;
    }

    connect(ui->btnMakeDefaultTemplate, &QPushButton::clicked, this,
            [&]()
            {
                rtm_.setDefaultItem( ui->lvTemplates->currentIndex() );
            }
    );

    connect(ui->btnAddTemplate, &QPushButton::clicked, this,
            [&]()
            {
                auto newpath = QFileDialog::getOpenFileName(
                            this,
                            "Please select report template file",
                            "",
                            "Executables (*.tex)"
                            );
                if (!newpath.isEmpty())
                {
                    boost::filesystem::path fp(newpath.toStdString());
                    rtm_.addItem(
                                insight::ResultReportTemplate(
                                    fp.filename().stem().string(), fp, false ) );
                }
            }
    );

    connect(ui->btnRemoveTemplate, &QPushButton::clicked, this,
            [&]()
            {
                rtm_.removeItem( ui->lvTemplates->currentIndex() );
            }
    );
}




IQManageReportTemplatesDialog::~IQManageReportTemplatesDialog()
{
    delete ui;
}




void IQManageReportTemplatesDialog::accept()
{
    auto listfile = insight::ResultReportTemplates::globalInstance().firstWritableLocation();

    if (!listfile.empty())
    {
        auto decision=QMessageBox::question(
              this,
              "Confirm writing",
              QString(boost::filesystem::exists(listfile) ?
                "Do you want to overwrite" : "Do you want to create")
              +" the file "+QString::fromStdString(listfile.string())+"?",
              QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);

        if ( decision == QMessageBox::Yes )
        {
            rtm_.configuration().writeConfiguration(listfile);
            insight::ResultReportTemplates::globalInstance().reload();
            QDialog::accept();
            return;
        }
        else if ( decision == QMessageBox::Cancel )
        {
            return;
        }
    }

    QMessageBox::critical(
          this,
          "Problem",
          "No suitable file location for the editied report templates list was found or accepted.\n"
          "The configuration could not be saved.",
          QMessageBox::Ok
          );
}

