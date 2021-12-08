#include "iqselectresultelementsdialog.h"
#include "ui_iqselectresultelementsdialog.h"

IQSelectResultElementsDialog::IQSelectResultElementsDialog(
        const insight::ResultSetPtr& resultSet,
        QWidget *parent ) :
    QDialog(parent),
    resultSet_(resultSet),
    resultSetModel_(resultSet_, true),
    ui(new Ui::IQSelectResultElementsDialog)
{
    ui->setupUi(this);
    ui->treeView->setModel(&resultSetModel_);
}

IQSelectResultElementsDialog::~IQSelectResultElementsDialog()
{
    delete ui;
}

QStringList IQSelectResultElementsDialog::filterEntries() const
{
    auto rsf = resultSetModel_.filter();
    QStringList fexp;
    for (const auto& e: rsf)
    {
        if (auto *s = boost::get<std::string>(&e))
        {
            fexp.append( QString::fromStdString(*s) );
        }
    }
    return fexp;
}
