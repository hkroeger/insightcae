#include "iqmanagereporttemplatesdialog.h"
#include "ui_iqmanagereporttemplatesdialog.h"

#include <QFileDialog>

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
                rtm_.setDefaultTemplate( ui->lvTemplates->currentIndex() );
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
                    rtm_.addTemplate(newpath);
                }
            }
    );

    connect(ui->btnRemoveTemplate, &QPushButton::clicked, this,
            [&]()
            {
                rtm_.removeTemplate( ui->lvTemplates->currentIndex() );
            }
    );
}

IQManageReportTemplatesDialog::~IQManageReportTemplatesDialog()
{
    delete ui;
}
