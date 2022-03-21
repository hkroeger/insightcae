#ifndef IQCONFIGUREEXTERNALPROGRAMSDIALOG_H
#define IQCONFIGUREEXTERNALPROGRAMSDIALOG_H

#include <QDialog>

#include "iqexternalprogramsmodel.h"

namespace Ui {
class IQConfigureExternalProgramsDialog;
}

class IQConfigureExternalProgramsDialog : public QDialog
{
    Q_OBJECT

    IQExternalProgramsModel model_;

public:
    explicit IQConfigureExternalProgramsDialog(QWidget *parent = nullptr);
    ~IQConfigureExternalProgramsDialog();

    const insight::ExternalPrograms& externalProgams() const;

    void accept() override;

private:
    Ui::IQConfigureExternalProgramsDialog *ui;
};

#endif // IQCONFIGUREEXTERNALPROGRAMSDIALOG_H
