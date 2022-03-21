#include "iqaddfilterdialog.h"
#include "ui_iqaddfilterdialog.h"

#include "iqselectresultelementsdialog.h"

#include <QInputDialog>

IQAddFilterDialog::IQAddFilterDialog(
        insight::ResultSetPtr resultSet,
        QWidget *parent
        ) :
    QDialog(parent),
    resultSet_(resultSet),
    ui(new Ui::IQAddFilterDialog)
{
    ui->setupUi(this);

    connect(ui->btnSelect, &QPushButton::clicked, this,
            [this]()
    {
        IQSelectResultElementsDialog dlg(resultSet_, this);
        if (dlg.exec()==QDialog::Accepted)
        {
            ui->tePattern->setText(
                        dlg.filterEntries().join("\n") );
        }
    }
    );
}

IQAddFilterDialog::~IQAddFilterDialog()
{
    delete ui;
}

insight::ResultSetFilter IQAddFilterDialog::filter() const
{
    insight::ResultSetFilter rsf;
    auto lines = ui->tePattern->document()->toPlainText().split("\n");
    for (const auto& line: lines)
    {
        if (ui->rbIsFixed->isChecked())
            rsf.insert(line.toStdString());
        else if (ui->rbIsRegex->isChecked())
            rsf.insert(boost::regex(line.toStdString()));
    }
    return rsf;
}
