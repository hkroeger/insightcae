#include "iqvtkcadmodel3dviewersettingsdialog.h"
#include "ui_iqvtkcadmodel3dviewersettingsdialog.h"

#include "iqvtkcadmodel3dviewer.h"

#include <QPushButton>
#include <boost/range/adaptor/indexed.hpp>

IQVTKCADModel3DViewerSettingsDialog::IQVTKCADModel3DViewerSettingsDialog(
    IQVTKCADModel3DViewer* viewer, QWidget *parent )
    : QDialog(parent)
    , ui(new Ui::IQVTKCADModel3DViewerSettingsDialog),
    viewer_(viewer)
{
    ui->setupUi(this);

    int curIdx=0;
    for (auto nm:
         boost::adaptors::index(VTKNavigationManager::navigationManagers()))
    {
        ui->cbNavigationStyle->addItem(
            QString::fromStdString(nm.value().first)
            );
        if (viewer_->currentNavigationManager().type()==nm.value().first)
        {
            curIdx=nm.index();
        }
    }
    ui->cbNavigationStyle->setCurrentIndex(curIdx);

    connect(ui->buttonBox->button(QDialogButtonBox::Apply),
            &QPushButton::clicked, this, &IQVTKCADModel3DViewerSettingsDialog::apply);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

IQVTKCADModel3DViewerSettingsDialog::~IQVTKCADModel3DViewerSettingsDialog()
{
    delete ui;
}

void IQVTKCADModel3DViewerSettingsDialog::accept()
{
    apply();
    QDialog::accept();
}

void IQVTKCADModel3DViewerSettingsDialog::apply()
{
    auto key=ui->cbNavigationStyle->currentText().toStdString();
    auto newnm=VTKNavigationManager::navigationManagers()(key, *viewer_);
    viewer_->resetNavigationManager(std::move(newnm));
}
