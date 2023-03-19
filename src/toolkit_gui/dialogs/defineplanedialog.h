#ifndef DEFINEPLANEDIALOG_H
#define DEFINEPLANEDIALOG_H

#include <QDialog>

#include "base/linearalgebra.h"

namespace Ui {
class DefinePlaneDialog;
}

class DefinePlaneDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DefinePlaneDialog(QWidget *parent = nullptr);
    ~DefinePlaneDialog();

    std::string label() const;
    arma::mat p0() const;
    arma::mat n() const;

private:
    Ui::DefinePlaneDialog *ui;
};

#endif // DEFINEPLANEDIALOG_H
