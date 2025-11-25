#include "iqselectresultelementsdialog.h"
#include "ui_iqselectresultelementsdialog.h"

IQSelectResultElementsDialog::IQSelectResultElementsDialog(
        const insight::ResultSet& resultSet,
        QWidget *parent ) :
    QDialog(parent),
#warning too expensive?
    resultSetModel_(resultSet.cloneAs<insight::ResultSet>(), true),
    ui(new Ui::IQSelectResultElementsDialog)
{
    ui->setupUi(this);
    ui->treeView->setModel(&resultSetModel_);
    ui->treeView->setAlternatingRowColors(true);
    ui->treeView->header()->setSectionResizeMode(
        QHeaderView::ResizeMode::ResizeToContents);

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
