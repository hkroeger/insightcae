#ifndef IQADDFILTERDIALOG_H
#define IQADDFILTERDIALOG_H

#include "toolkit_gui_export.h"

#include <QDialog>
#include "base/resultset.h"
#include "base/resultsetfilter.h"

namespace Ui {
class IQAddFilterDialog;
}

class TOOLKIT_GUI_EXPORT IQAddFilterDialog
        : public QDialog
{
    Q_OBJECT

    insight::ResultSetPtr resultSet_;

public:
    explicit IQAddFilterDialog(
            insight::ResultSetPtr resultSet,
            QWidget *parent = nullptr
            );
    ~IQAddFilterDialog();

    insight::ResultSetFilter filter() const;

private:
    Ui::IQAddFilterDialog *ui;
};

#endif // IQADDFILTERDIALOG_H
