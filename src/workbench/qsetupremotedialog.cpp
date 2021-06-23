#include "qsetupremotedialog.h"
#include "ui_qsetupremotedialog.h"

#include "remotedirselector.h"

#include <QMessageBox>

QSetupRemoteDialog::QSetupRemoteDialog(const QString& hostName, const QString& path, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::QSetupRemoteDialog)
{
  ui->setupUi(this);

  for (const auto& i: insight::remoteServers)
  {
    ui->cbHost->addItem( QString::fromStdString(*i) );
  }

  if (!hostName.isEmpty())
  {
    ui->cbHost->setCurrentIndex(
        ui->cbHost->findText( hostName )
        );
  }

  ui->leRemoteDirectory->setText(path);

  connect(ui->btnSelectRemoteDirectory, &QPushButton::clicked,
          [&]()
          {
              auto i = insight::remoteServers.findServer(
                    ui->cbHost->currentText().toStdString()
                    );

              if (i->isDynamicallyAllocated())
              {
                QMessageBox::critical(this,
                                      "Error",
                                      "Selected host is an on-demand host.\n"
                                      "Selection of remote directories is only supported on permanent hosts.");
                return;
              }
              else
              {
                RemoteDirSelector dlg( this, i->second.server_ );
                if (dlg.exec() == QDialog::Accepted)
                {
                    ui->cbHost->setCurrentIndex(
                          ui->cbHost->findText( QString::fromStdString(dlg.selectedServer()) )
                          );
                    ui->leRemoteDirectory->setText( QString::fromStdString(dlg.selectedRemoteDir().string()) );
                }
              }
          }
  );
}

QString QSetupRemoteDialog::hostName() const
{
    return ui->cbHost->currentText();
}

QString QSetupRemoteDialog::remoteDirectory() const
{
    return ui->leRemoteDirectory->text();
}

QSetupRemoteDialog::~QSetupRemoteDialog()
{
  delete ui;
}
