#ifndef IQCADEXCEPTIONDISPLAYDIALOG_H
#define IQCADEXCEPTIONDISPLAYDIALOG_H

#include <QDialog>

#include "cadexception.h"

namespace Ui {
class IQCADExceptionDisplayDialog;
}

class IQVTKISCADModelDisplay;

class IQCADExceptionDisplayDialog : public QDialog
{
    Q_OBJECT

    IQVTKISCADModelDisplay *display_;

public:
    explicit IQCADExceptionDisplayDialog(QWidget *parent = nullptr);
    ~IQCADExceptionDisplayDialog();

    void displayException(const insight::CADErrorDescription &e);

private:
    Ui::IQCADExceptionDisplayDialog *ui;
};

#endif // IQCADEXCEPTIONDISPLAYDIALOG_H
