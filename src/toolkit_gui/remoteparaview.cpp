#include "remoteparaview.h"
#include "ui_remoteparaview.h"

#include <QSettings>
#include <QProcess>
#include <QMessageBox>
#include <QFileDialog>

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

  ui->statefile->clear();
  ui->statefile->addItems( settings.value("recentstatefiles").toStringList() );
  ui->statefile->setCurrentText( settings.value("statefile").toString() );

  connect( ui->btnSelectStateFile, &QPushButton::clicked,
           [&]()
  {
    QString fn = QFileDialog::getOpenFileName(
          this, "Select PV state file", "", "ParaView State File (*.pvsm)"
          );
    if (!fn.isEmpty())
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
              auto rem_host = ui->remhost->text();
              auto statefile = ui->statefile->currentText();

              if (!rem_subdir.isEmpty())
                args << "-s" << rem_subdir;

              if (!rem_host.isEmpty())
                args << "-r" << rem_host;

              if (!statefile.isEmpty())
                args << "-t" << statefile;

              if (!QProcess::startDetached("isPVRemote.sh", args,
                                           QString::fromStdString(rec_.localDir().string()) ))
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

  QString csf=ui->statefile->currentText();
  QStringList rsfi;
  for (int i=0; i<std::min(25,ui->statefile->count()); i++)
    rsfi.append(ui->statefile->itemText(i));

  if (!rsfi.contains(csf)) rsfi.prepend(csf);

  settings.setValue("recentstatefiles", rsfi);
  settings.setValue("statefile", csf);

  delete ui;
}
