#include "defineplanedialog.h"
#include "ui_defineplanedialog.h"

#include "base/tools.h"

DefinePlaneDialog::DefinePlaneDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DefinePlaneDialog)
{
    ui->setupUi(this);
}

DefinePlaneDialog::~DefinePlaneDialog()
{
    delete ui;
}

std::string DefinePlaneDialog::label() const
{
    return ui->planeLabel->text().toStdString();
}

arma::mat DefinePlaneDialog::p0() const
{
    insight::CurrentExceptionContext ex("converting point coordinate strings into numbers");
    return insight::vec3(
                insight::toNumber(ui->p_x->text().toStdString()),
                insight::toNumber(ui->p_y->text().toStdString()),
                insight::toNumber(ui->p_z->text().toStdString())
                );
}

arma::mat DefinePlaneDialog::n() const
{
    insight::CurrentExceptionContext ex("converting normal vector coordinate strings into numbers");
    return insight::normalized(insight::vec3(
                insight::toNumber(ui->n_x->text().toStdString()),
                insight::toNumber(ui->n_y->text().toStdString()),
                insight::toNumber(ui->n_z->text().toStdString())
                ));
}
