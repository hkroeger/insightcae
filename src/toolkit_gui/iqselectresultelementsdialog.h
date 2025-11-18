#ifndef IQSELECTRESULTELEMENTSDIALOG_H
#define IQSELECTRESULTELEMENTSDIALOG_H

#include "toolkit_gui_export.h"

#include <QDialog>

#include "base/resultset.h"
#include "iqresultsetmodel.h"

namespace Ui {
class IQSelectResultElementsDialog;
}

class TOOLKIT_GUI_EXPORT IQSelectResultElementsDialog : public QDialog
{
    Q_OBJECT

    insight::IQResultSetModel resultSetModel_;

public:
    explicit IQSelectResultElementsDialog(
            const insight::ResultSet& resultSet,
            QWidget *parent = nullptr );

    ~IQSelectResultElementsDialog();

    QStringList filterEntries() const;

private:
    Ui::IQSelectResultElementsDialog *ui;
};

#endif // IQSELECTRESULTELEMENTSDIALOG_H
