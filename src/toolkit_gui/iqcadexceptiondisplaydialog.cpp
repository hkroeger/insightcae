#include "iqcadexceptiondisplaydialog.h"
#include "qnamespace.h"
#include "ui_iqcadexceptiondisplaydialog.h"

#include "iqcaditemmodel.h"
#include "iqvtkcadmodel3dviewer.h"
#include "iqvtkiscadmodeldisplay.h"

#include <QTreeView>
#include <QHBoxLayout>
#include <QSplitter>

//#include "base/translations.h"

IQCADExceptionDisplayDialog::IQCADExceptionDisplayDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::IQCADExceptionDisplayDialog)
{
    ui->setupUi(this);

    auto model = new IQCADItemModel;
    auto viewer = new IQVTKCADModel3DViewer;
    auto tree = new QTreeView;
    display_ = new IQVTKISCADModelDisplay(
        ui->display, model,
        viewer, tree );

    auto *l = new QHBoxLayout;
    ui->display->setLayout(l);
    auto spl=new QSplitter(Qt::Horizontal);
    l->addWidget(spl);
    spl->addWidget(viewer);
    spl->addWidget(tree);
}

IQCADExceptionDisplayDialog::~IQCADExceptionDisplayDialog()
{
    delete ui;
}


void IQCADExceptionDisplayDialog::displayException(
    const insight::CADException &e )
{
    ui->message->setText(
       QString("An error has occurred:")
      +"\n"
      +QString::fromStdString(e.as_string())
      );

    for (const auto& cg: e.contextGeometry())
    {
        display_->model()->addModelstep(
            cg.first, cg.second, true
            );
    }
}
