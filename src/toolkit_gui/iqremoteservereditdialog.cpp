#include "iqremoteservereditdialog.h"
#include "ui_iqremoteservereditdialog.h"

#include "iqselectremotehosttypedialog.h"

#include "base/tools.h"

#include <QMessageBox>

IQRemoteServerEditDialog::IQRemoteServerEditDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::IQRemoteServerEditDialog)
{
  ui->setupUi(this);
  ui->lwServers->setModel(&serverListModel_);

  connect(ui->lwServers->selectionModel(), &QItemSelectionModel::currentChanged, this,
          [&](const QModelIndex &current, const QModelIndex &/*previous*/)
          {
            ui->btnEdit->setEnabled(current.isValid());
            ui->btnRemove->setEnabled(current.isValid());
          }
  );

  connect(ui->btnAdd, &QPushButton::clicked, this,
          [&]()
          {
            IQSelectRemoteHostTypeDialog dlg(serverListModel_.remoteServers(), insight::RemoteServer::ConfigPtr(), this);
            if (dlg.exec()==QDialog::Accepted)
            {
              serverListModel_.addRemoteServer(dlg.result_);
            }
          }
  );

  connect(ui->btnEdit, &QPushButton::clicked, this,
          [&]()
          {
            auto index = ui->lwServers->currentIndex();
            if (index.isValid())
            {
              auto rsc = serverListModel_.getRemoteServer(index);
              IQSelectRemoteHostTypeDialog dlg(serverListModel_.remoteServers(), *rsc, this);
              if (dlg.exec()==QDialog::Accepted)
              {
                serverListModel_.removeRemoteServer(rsc);
                serverListModel_.addRemoteServer(dlg.result_);
              }
            }
          }
  );

  connect(ui->btnRemove, &QPushButton::clicked, this,
          [&]()
          {
            auto index = ui->lwServers->currentIndex();
            if (index.isValid())
            {
              auto rsc = serverListModel_.getRemoteServer(index);
              if (QMessageBox::question(
                    this,
                    "Confirm",
                    "Really delete server configuration "+QString::fromStdString(**rsc)+"?")
                  ==QMessageBox::Yes)
              {
                serverListModel_.removeRemoteServer(rsc);
              }
            }
          }
  );

}


IQRemoteServerEditDialog::~IQRemoteServerEditDialog()
{
  delete ui;
}


void IQRemoteServerEditDialog::accept()
{
  auto serverlist = insight::remoteServers.firstWritableLocation();

  if (!serverlist.empty())
  {
      auto decision=QMessageBox::question(
            this,
            "Confirm writing",
            QString(boost::filesystem::exists(serverlist) ?
              "Do you want to overwrite" : "Do you want to create")
            +" the file "+QString::fromStdString(serverlist.string())+"?",
            QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);

      if ( decision == QMessageBox::Yes )
      {
        insight::remoteServers = serverListModel_.remoteServers();
        insight::remoteServers.writeConfiguration(serverlist);
        QDialog::accept();
        return;
      }
      else if ( decision == QMessageBox::Cancel )
      {
        return;
      }
  }

  QMessageBox::critical(
        this,
        "Problem",
        "No suitable file location for the editied server list was found or accepted.\n"
        "The configuration could not be saved.",
        QMessageBox::Ok
        );
}
