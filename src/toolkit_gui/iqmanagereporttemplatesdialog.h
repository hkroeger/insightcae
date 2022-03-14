#ifndef IQMANAGEREPORTTEMPLATESDIALOG_H
#define IQMANAGEREPORTTEMPLATESDIALOG_H

#include <QDialog>
#include "iqreporttemplatesmodel.h"

namespace Ui {
class IQManageReportTemplatesDialog;
}

class IQManageReportTemplatesDialog : public QDialog
{
    Q_OBJECT

    IQReportTemplatesModel rtm_;

public:
    explicit IQManageReportTemplatesDialog(QWidget *parent = nullptr);
    ~IQManageReportTemplatesDialog();

    void accept() override;

private:
    Ui::IQManageReportTemplatesDialog *ui;
};

#endif // IQMANAGEREPORTTEMPLATESDIALOG_H
