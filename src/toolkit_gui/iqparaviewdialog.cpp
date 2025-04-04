#include "iqparaviewdialog.h"
#include "ui_iqparaviewdialog.h"
#include "qtextensions.h"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>




IQParaviewDialog::IQParaviewDialog(
        const boost::filesystem::path& caseDirectory,
        QWidget *parent ) :
    QDialog(parent),
    caseDirectory_(caseDirectory),
    ui(new Ui::IQParaviewDialog)
{
    ui->setupUi(this);

    QSettings settings("silentdynamics", "isofExecutionManager");

    ui->statefile->clear();
    ui->statefile->addItems( settings.value("recentstatefiles").toStringList() );
    ui->statefile->setCurrentText( settings.value("statefile").toString() );

    connect( ui->btnSelectStateFile, &QPushButton::clicked, ui->btnSelectStateFile,
             [&]()
    {
      if (auto fn = getFileName(
            this, "Select PV state file",
            GetFileMode::Open,
            {{ "pvsm", "ParaView State File" }}
                ) )
      {
        ui->statefile->setCurrentText(fn);
      }
    }
    );

    connect( ui->buttonBox, &QDialogButtonBox::accepted,
             [&]()
             {
                QStringList args;

                auto statefile = ui->statefile->currentText();

                bool batch = ui->cbBatch->isChecked();
                bool onlylatesttime = ui->cbOnlyLatestTime->isChecked();
                double from = ui->leFrom->text().toDouble();
                double to = ui->leTo->text().toDouble();
                bool rescale = ui->cbRescale->isChecked();

                pv_ = std::make_shared<insight::Paraview>(
                            caseDirectory_, statefile.toStdString(),
                            batch, false, rescale, onlylatesttime,
                            from, to
                            );
             }
    );
}




IQParaviewDialog::~IQParaviewDialog()
{
    QSettings settings("silentdynamics", "isofExecutionManager");

    QString csf=ui->statefile->currentText();
    QStringList rsfi;
    for (int i=0; i<std::min(25,ui->statefile->count()); i++)
      rsfi.append(ui->statefile->itemText(i));

    if (!rsfi.contains(csf)) rsfi.prepend(csf);

    settings.setValue("recentstatefiles", rsfi);
    settings.setValue("statefile", csf);

    delete ui;
}




std::shared_ptr<insight::Paraview> IQParaviewDialog::paraviewProcess()
{
    return pv_;
}
