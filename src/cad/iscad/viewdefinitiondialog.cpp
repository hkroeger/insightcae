#include "viewdefinitiondialog.h"
#include "ui_viewdefinitiondialog.h"

ViewDefinitionDialog::ViewDefinitionDialog(
    QWidget *parent,
    const DrawingViewDefinition& vd )
    : QDialog(parent)
    , ui(new Ui::ViewDefinitionDialog)
{
    ui->setupUi(this);

#define SET(F, T) \
    ui->T->setText( \
        QString::fromStdString(vd.F))

    SET(label, viewLabel);
    SET(onPointExpr, onExpr);
    SET(normalExpr, normalExpr);
    SET(upExpr, upExpr);
    ui->specifyUp->setChecked(!vd.upExpr.empty());
    ui->sec->setChecked(vd.isSection);
    ui->skipHL->setChecked(vd.skipHL);
    ui->addL->setChecked(vd.add.count(AddedView::l));
    ui->addR->setChecked(vd.add.count(AddedView::r));
    ui->addT->setChecked(vd.add.count(AddedView::t));
    ui->addB->setChecked(vd.add.count(AddedView::b));
    ui->addK->setChecked(vd.add.count(AddedView::k));

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ViewDefinitionDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ViewDefinitionDialog::reject);
}

ViewDefinitionDialog::~ViewDefinitionDialog()
{
    delete ui;
}

void ViewDefinitionDialog::accept()
{
#define GET(F, T) \
    viewDef_.F = ui->T->text().toStdString()

    GET(label, viewLabel);
    GET(onPointExpr, onExpr);
    GET(normalExpr, normalExpr);
    if (ui->specifyUp->isChecked())
        GET(upExpr, upExpr);
    viewDef_.isSection=ui->sec->isChecked();
    viewDef_.skipHL=ui->skipHL->isChecked();
    viewDef_.add.clear();
    if (ui->addL->isChecked()) viewDef_.add.insert(AddedView::l);
    if (ui->addR->isChecked()) viewDef_.add.insert(AddedView::r);
    if (ui->addT->isChecked()) viewDef_.add.insert(AddedView::t);
    if (ui->addB->isChecked()) viewDef_.add.insert(AddedView::b);
    if (ui->addK->isChecked()) viewDef_.add.insert(AddedView::k);
    QDialog::accept();
}

const DrawingViewDefinition &ViewDefinitionDialog::viewDef() const
{
    return viewDef_;
}
