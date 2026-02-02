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
    explicit OrientBackgroundImageCoordinatesDialog(
        const arma::mat& xy1, const arma::mat& xy2,
        QWidget *parent = nullptr);

    ~OrientBackgroundImageCoordinatesDialog();

    arma::mat xy1() const;
    arma::mat xy2() const;

private:
    Ui::OrientBackgroundImageCoordinatesDialog *ui;
};

#endif // ORIENTBACKGROUNDIMAGECOORDINATESDIALOG_H
