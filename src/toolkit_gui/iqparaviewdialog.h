#ifndef IQPARAVIEWDIALOG_H
#define IQPARAVIEWDIALOG_H

#include "toolkit_gui_export.h"
#include "openfoam/paraview.h"

#include <QDialog>

namespace Ui {
class IQParaviewDialog;
}

class TOOLKIT_GUI_EXPORT IQParaviewDialog : public QDialog
{
    Q_OBJECT

    boost::filesystem::path caseDirectory_;
    std::shared_ptr<insight::Paraview> pv_;

public:
    explicit IQParaviewDialog(
            const boost::filesystem::path& caseDirectory,
            QWidget *parent = nullptr );
    ~IQParaviewDialog();

    std::shared_ptr<insight::Paraview> paraviewProcess();

private:
    Ui::IQParaviewDialog *ui;
};

#endif // IQPARAVIEWDIALOG_H
