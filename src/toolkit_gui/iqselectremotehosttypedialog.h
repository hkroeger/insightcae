#ifndef IQSELECTREMOTEHOSTTYPEDIALOG_H
#define IQSELECTREMOTEHOSTTYPEDIALOG_H

#include "toolkit_gui_export.h"
#include "base/remoteserverlist.h"

#include <QDialog>
#include <QLineEdit>


struct ServerSetup
{
  QWidget* parent_;
  ServerSetup(QWidget* parent);
  virtual ~ServerSetup();
  virtual insight::RemoteServer::ConfigPtr result() =0;
};


struct SSHLinuxSetup
    : public ServerSetup
{
  QLineEdit *leHostName_, *leBaseDir_;

  SSHLinuxSetup(QWidget* parent, insight::RemoteServer::ConfigPtr initialcfg = insight::RemoteServer::ConfigPtr() );

  insight::RemoteServer::ConfigPtr result() override;
};

struct WSLLinuxSetup
    : public ServerSetup
{
  QLineEdit *leDistributionLabel_, *leBaseDir_;

  WSLLinuxSetup(QWidget* parent, insight::RemoteServer::ConfigPtr initialcfg = insight::RemoteServer::ConfigPtr() );

  insight::RemoteServer::ConfigPtr result() override;
};


namespace Ui {
class IQSelectRemoteHostTypeDialog;
}

class TOOLKIT_GUI_EXPORT IQSelectRemoteHostTypeDialog : public QDialog
{
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
