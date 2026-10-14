#include "iqremoteservereditdialog.h"
#include "ui_iqremoteservereditdialog.h"

#include "iqselectremotehosttypedialog.h"
#include "iqserverlaunchdialog.h"

#include "base/translations.h"

#include <QHeaderView>

#include "base/tools.h"

#include <QMessageBox>

IQRemoteServerEditDialog::IQRemoteServerEditDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::IQRemoteServerEditDialog)
{
  ui->setupUi(this);
  ui->lwServers->setModel(&serverListModel_);
  ui->lwServers->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->lwServers->header()->setSectionResizeMode(
      IQRemoteServerListModel::NameColumn, QHeaderView::ResizeToContents );
  ui->lwServers->header()->setSectionResizeMode(
      IQRemoteServerListModel::StatusColumn, QHeaderView::Stretch );

  connect(ui->lwServers->selectionModel(), &QItemSelectionModel::currentChanged, this,
          [&](const QModelIndex &current, const QModelIndex &/*previous*/)
          {
            ui->btnEdit->setEnabled(current.isValid());
            ui->btnRemove->setEnabled(current.isValid());
            ui->btnSetPreferred->setEnabled(current.isValid());
            updateServerActionButtons();
          }
  );

  // the state of the servers is determined in background
  connect(&serverListModel_, &QAbstractItemModel::dataChanged, this,
          [this]() { updateServerActionButtons(); } );
  connect(&serverListModel_, &QAbstractItemModel::rowsRemoved, this,
          [this]() { updateServerActionButtons(); } );

  connect(ui->btnLaunch, &QPushButton::clicked, this,
          [&]()
          {
            auto index = ui->lwServers->currentIndex();
            if (!index.isValid()) return;
            auto rsc = serverListModel_.getRemoteServer(index);
            IQServerLaunchDialog::launchServer(*rsc, this); // displays errors itself
            serverListModel_.checkStatus(index);
          }
  );

  connect(ui->btnShutdown, &QPushButton::clicked, this,
          [&]()
          {
            auto index = ui->lwServers->currentIndex();
            if (!index.isValid()) return;
            auto rsc = serverListModel_.getRemoteServer(index);

            auto answer = QMessageBox::question(
                this, _("Shut down server"),
                QString(_("Shut down the server %1?\n\n"
                          "Analyses, which are currently running on it, will be terminated"
                          " and data on its local storage might be lost."))
                    .arg(QString::fromStdString(*rsc)),
                QMessageBox::Yes|QMessageBox::No, QMessageBox::No );
            if (answer!=QMessageBox::Yes) return;

            IQServerLaunchDialog::shutdownServer(*rsc, this); // displays errors itself
            serverListModel_.checkStatus(index);
          }
  );

  connect(ui->btnDispose, &QPushButton::clicked, this,
          [&]()
          {
            auto index = ui->lwServers->currentIndex();
            if (!index.isValid()) return;
            auto rsc = serverListModel_.getRemoteServer(index);

            auto answer = QMessageBox::warning(
                this, _("Dispose server"),
                QString(_("Shut down the server %1 and delete its instance?\n\n"
                          "Analyses, which are currently running on it, will be terminated"
                          " and all data on the instance will be deleted!"))
                    .arg(QString::fromStdString(*rsc)),
                QMessageBox::Yes|QMessageBox::No, QMessageBox::No );
            if (answer!=QMessageBox::Yes) return;

            IQServerLaunchDialog::disposeServer(*rsc, this); // displays errors itself
            serverListModel_.checkStatus(index);
          }
  );

  connect(ui->btnRefreshStatus, &QPushButton::clicked,
          &serverListModel_, &IQRemoteServerListModel::checkAllStatus );

  connect(ui->btnAdd, &QPushButton::clicked, this,
          [&]()
          {
            IQSelectRemoteHostTypeDialog dlg(
                serverListModel_.remoteServers(),
                insight::RemoteServer::ConfigPtr(),
                this );

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
              IQSelectRemoteHostTypeDialog dlg(serverListModel_.remoteServers(), rsc, this);
              if (dlg.exec()==QDialog::Accepted)
              {
                serverListModel_.removeRemoteServer(index);
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
                    "Really delete server configuration "+QString::fromStdString(*rsc)+"?")
                  ==QMessageBox::Yes)
              {
                serverListModel_.removeRemoteServer(index);
              }
            }
          }
  );

  connect(ui->btnSetPreferred, &QPushButton::clicked, this,
          [&]()
          {
              auto index = ui->lwServers->currentIndex();
              if (index.isValid())
              {
                  serverListModel_.setPreferredServer(index);
              }
          }
  );

  serverListModel_.checkAllStatus();
}




void IQRemoteServerEditDialog::updateServerActionButtons()
{
  auto index = ui->lwServers->currentIndex();

  bool isServer = index.isValid() && !serverListModel_.isServerPool(index);
  auto online = serverListModel_.isOnline(index);
  auto exists = serverListModel_.exists(index);

  insight::RemoteServer::ConfigPtr rsc;
  if (isServer) rsc = serverListModel_.getRemoteServer(index);

  // launch: only a server, which is down and can be created
  ui->btnLaunch->setEnabled(
      rsc && online && !*online && rsc->canBeLaunched() );
  ui->btnLaunch->setToolTip(
      !rsc ? _("Select a server (not a server pool).") :
      !online ? _("Checking, if the server is reachable...") :
      *online ? _("The server is running.") :
      !rsc->canBeLaunched() ?
          _("The server cannot be launched from here (no launch or creation command configured).") :
      (exists && !*exists) ?
          _("Create the instance and launch the server (execute its creation command).") :
          _("Launch the server (execute its launch command; create its instance before, if it does not exist).") );

  // shutdown: only a running server, which can be destroyed
  ui->btnShutdown->setEnabled(
      rsc && online && *online && rsc->canBeShutDown() );
  ui->btnShutdown->setToolTip(
      !rsc ? _("Select a server (not a server pool).") :
      !online ? _("Checking, if the server is reachable...") :
      !*online ? _("The server is not running.") :
      !rsc->canBeShutDown() ?
          _("The server cannot be shut down from here (no shutdown command configured).") :
          _("Shut down the server (execute its shutdown command).") );

  // dispose: a running or existing instance, which can be deleted
  bool instancePresent = (online && *online) || (exists && *exists);
  ui->btnDispose->setEnabled(
      rsc && rsc->canBeDisposed() && instancePresent );
  ui->btnDispose->setToolTip(
      !rsc ? _("Select a server (not a server pool).") :
      !rsc->canBeDisposed() ?
          _("The server instance cannot be disposed from here (no destruction command configured).") :
      !online ? _("Checking, if the server is reachable...") :
      (exists && !*exists) ? _("The instance does not exist.") :
      !instancePresent ? _("Checking, if the instance exists...") :
          _("Shut down the server (if running) and delete its instance (execute its destruction command).") );
}


IQRemoteServerEditDialog::~IQRemoteServerEditDialog()
{
  delete ui;
}


void IQRemoteServerEditDialog::accept()
{
  auto serverlist = insight::remoteServers().firstWritableLocation();

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
        insight::remoteServers()= serverListModel_.remoteServers();
        insight::remoteServers().writeConfiguration(serverlist);
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
