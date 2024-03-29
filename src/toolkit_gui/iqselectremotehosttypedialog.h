#ifndef IQSELECTREMOTEHOSTTYPEDIALOG_H
#define IQSELECTREMOTEHOSTTYPEDIALOG_H

#include "toolkit_gui_export.h"
#include "base/remoteserverlist.h"

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>


namespace Ui {
class IQSelectRemoteHostTypeDialog;
}

class IQSelectRemoteHostTypeDialog;

struct ServerSetup
{
  IQSelectRemoteHostTypeDialog *dlg_;
  QWidget* parent_;
  ServerSetup(QWidget* parent, IQSelectRemoteHostTypeDialog *dlg);
  virtual ~ServerSetup();
  virtual insight::RemoteServer::ConfigPtr result() =0;
  Ui::IQSelectRemoteHostTypeDialog* dlgui();
};


struct SSHLinuxSetup
    : public ServerSetup
{
  QLineEdit *leHostName_, *leBaseDir_;

  SSHLinuxSetup(QWidget* parent, IQSelectRemoteHostTypeDialog *dlg, insight::RemoteServer::ConfigPtr initialcfg = insight::RemoteServer::ConfigPtr() );

  insight::RemoteServer::ConfigPtr result() override;
};

struct WSLLinuxSetup
    : public ServerSetup
{
  QLineEdit *leBaseDir_;
  QComboBox *leDistributionLabel_;

  WSLLinuxSetup(QWidget* parent, IQSelectRemoteHostTypeDialog *dlg, insight::RemoteServer::ConfigPtr initialcfg = insight::RemoteServer::ConfigPtr() );

  insight::RemoteServer::ConfigPtr result() override;
};



class TOOLKIT_GUI_EXPORT IQSelectRemoteHostTypeDialog : public QDialog
{
  friend class ServerSetup;

  Q_OBJECT

  insight::RemoteServerList& remoteServers_;

  std::unique_ptr<ServerSetup> setupControls_;

  bool entryShouldNotExist_;

public:
  insight::RemoteServer::ConfigPtr result_;

  explicit IQSelectRemoteHostTypeDialog(insight::RemoteServerList& remoteServers, insight::RemoteServer::ConfigPtr cfgToEdit, QWidget *parent = nullptr);
  ~IQSelectRemoteHostTypeDialog();

  void accept() override;

private:
  Ui::IQSelectRemoteHostTypeDialog *ui;
};


#endif // IQSELECTREMOTEHOSTTYPEDIALOG_H
