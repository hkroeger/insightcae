#include "remoteparaview.h"
#include "ui_remoteparaview.h"

#include <QSettings>
#include <QProcess>
#include <QMessageBox>

RemoteParaview::RemoteParaview
(
    const insight::RemoteExecutionConfig& rec,
    QWidget *parent
) :
  QDialog(parent),
  rec_(rec),
  ui(new Ui::RemoteParaview)
{
  QSettings settings("silentdynamics", "isofExecutionManager");

  ui->setupUi(this);

  auto rds=rec_.remoteSubdirs();
  for (auto d: rds)
    ui->subdir->addItem( QString::fromStdString(d.string()) );

  ui->remhost->setText( settings.value("remhost").toString() );
  ui->subdir->setCurrentText( settings.value("remsubdir").toString() );

  connect( ui->buttonBox, &QDialogButtonBox::accepted,
           [&]()
           {
              QStringList args;

              auto rem_subdir = ui->subdir->currentText();
              auto rem_host = ui->remhost->text();

              if (!rem_subdir.isEmpty())
                args << "-s" << rem_subdir;

              if (!rem_host.isEmpty())
                args << "-r" << rem_host;

              if (!QProcess::startDetached("isPVRemote.sh", args, rec_.localDir().c_str() ))
              {
                QMessageBox::critical(
                      this,
                      "Failed to start",
                      QString("Failed to start Paraview in directoy ")+rec_.localDir().c_str()
                      );
              }
           }
  );
}

RemoteParaview::~RemoteParaview()
{
  QSettings settings("silentdynamics", "isofExecutionManager");
  settings.setValue("remhost", ui->remhost->text());
  settings.setValue("remsubdir", ui->subdir->currentText());
  delete ui;
}
