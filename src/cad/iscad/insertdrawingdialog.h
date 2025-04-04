#ifndef INSERTDRAWINGDIALOG_H
#define INSERTDRAWINGDIALOG_H

#include <QDialog>
#include "drawingviewsmodel.h"

namespace Ui {
class InsertDrawingDialog;
}

class InsertDrawingDialog : public QDialog
{
    Q_OBJECT


    std::string expression_;
    DrawingViewsModel views_;

public:
    explicit InsertDrawingDialog(QWidget *parent = nullptr);
    ~InsertDrawingDialog();

    void accept() override;

    std::string expression() const;

private:
    Ui::InsertDrawingDialog *ui;
};

#endif // INSERTDRAWINGDIALOG_H
