#include "iqremoteparaviewdialog.h"
#include "ui_iqremoteparaviewdialog.h"
#include "qtextensions.h"

#include <QSettings>
#include <QProcess>
#include <QMessageBox>
#include <QFileDialog>




IQRemoteParaviewDialog::IQRemoteParaviewDialog
(
    const insight::RemoteExecutionConfig& rec,
    QWidget *parent
) :
  QDialog(parent),
  rec_(rec),
  ui(new Ui::IQRemoteParaviewDialog)
{
  QSettings settings("silentdynamics", "isofExecutionManager");

  ui->setupUi(this);

  auto rds=rec_.remoteSubdirs();
  for (auto d: rds)
    ui->subdir->addItem( QString::fromStdString(d.string()) );

  ui->subdir->setCurrentText( settings.value("remsubdir").toString() );

  ui->statefile->clear();
  ui->statefile->addItems( settings.value("recentstatefiles").toStringList() );
  ui->statefile->setCurrentText( settings.value("statefile").toString() );

  connect( ui->btnSelectStateFile, &QPushButton::clicked,
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

              auto rem_subdir = ui->subdir->currentText();
              auto statefile = ui->statefile->currentText();

              rp_ = std::make_shared<insight::RemoteParaview>(
                          rec_,
                          statefile.toStdString(),
                          rem_subdir.toStdString()
                          );
           }
  );
}




IQRemoteParaviewDialog::~IQRemoteParaviewDialog()
{
  QSettings settings("silentdynamics", "isofExecutionManager");
  settings.setValue("remsubdir", ui->subdir->currentText());

  QString csf=ui->statefile->currentText();
  QStringList rsfi;
  for (int i=0; i<std::min(25,ui->statefile->count()); i++)
    rsfi.append(ui->statefile->itemText(i));

  if (!rsfi.contains(csf)) rsfi.prepend(csf);

  settings.setValue("recentstatefiles", rsfi);
  settings.setValue("statefile", csf);

  delete ui;
}




std::shared_ptr<insight::RemoteParaview> IQRemoteParaviewDialog::remoteParaviewProcess()
{
    return rp_;
}
