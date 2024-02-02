#ifndef ORIENTBACKGROUNDIMAGECOORDINATESDIALOG_H
#define ORIENTBACKGROUNDIMAGECOORDINATESDIALOG_H

#include <QDialog>

#include <armadillo>

namespace Ui {
class OrientBackgroundImageCoordinatesDialog;
}

class OrientBackgroundImageCoordinatesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OrientBackgroundImageCoordinatesDialog(const arma::mat& p1, const arma::mat& p2, QWidget *parent = nullptr);
    ~OrientBackgroundImageCoordinatesDialog();

    arma::mat p1() const;
    arma::mat p2() const;

private:
    Ui::OrientBackgroundImageCoordinatesDialog *ui;
};

#endif // ORIENTBACKGROUNDIMAGECOORDINATESDIALOG_H
