#include "orientbackgroundimagecoordinatesdialog.h"
#include "ui_orientbackgroundimagecoordinatesdialog.h"

#include "base/linearalgebra.h"

OrientBackgroundImageCoordinatesDialog::OrientBackgroundImageCoordinatesDialog(
    const arma::mat& xy1, const arma::mat& xy2, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OrientBackgroundImageCoordinatesDialog)
{
    ui->setupUi(this);
    ui->x1->setText(QString::number(xy1[0]));
    ui->y1->setText(QString::number(xy1[1]));
    ui->x2->setText(QString::number(xy2[0]));
    ui->y2->setText(QString::number(xy2[1]));
}

OrientBackgroundImageCoordinatesDialog::~OrientBackgroundImageCoordinatesDialog()
{
    delete ui;
}


arma::mat OrientBackgroundImageCoordinatesDialog::xy1() const
{
    bool ok1, ok2;
    arma::mat p = insight::vec3(
        ui->x1->text().toDouble(&ok1),
        ui->y1->text().toDouble(&ok2), 0 );
    insight::assertion(ok1, "coordinate x1 is not a number!");
    insight::assertion(ok1, "coordinate y1 is not a number!");
    return p;
}


arma::mat OrientBackgroundImageCoordinatesDialog::xy2() const
{
    bool ok1, ok2;
    arma::mat p = insight::vec3(
        ui->x2->text().toDouble(&ok1),
        ui->y2->text().toDouble(&ok2), 0 );
    insight::assertion(ok1, "coordinate x2 is not a number!");
    insight::assertion(ok1, "coordinate y2 is not a number!");
    return p;
}
