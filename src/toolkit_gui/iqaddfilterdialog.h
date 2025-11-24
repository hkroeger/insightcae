#ifndef IQADDFILTERDIALOG_H
#define IQADDFILTERDIALOG_H

#include "toolkit_gui_export.h"

#include <QDialog>
#include "base/resultset.h"
#include "base/hierarchicaldatafilter.h"

namespace Ui {
class IQAddFilterDialog;
}

class TOOLKIT_GUI_EXPORT IQAddFilterDialog
        : public QDialog
{
    Q_OBJECT

    const insight::ResultSet& resultSet_;

public:
    explicit IQAddFilterDialog(
            const insight::ResultSet& resultSet,
            QWidget *parent = nullptr
            );
    ~IQAddFilterDialog();

    insight::hierarchicalData::Filter filter() const;

private:
    Ui::IQAddFilterDialog *ui;
};

#endif // IQADDFILTERDIALOG_H
