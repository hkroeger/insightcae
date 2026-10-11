#ifndef QEXECUTIONENVIRONMENTDIALOG_H
#define QEXECUTIONENVIRONMENTDIALOG_H

#include <QDialog>

#include <map>
#include <memory>

#include "base/tools.h"
#include "base/casedirectory.h"
#include "base/remoteexecution.h"

namespace Ui {
class QExecutionWorkspaceDialog;
}

class QExecutionWorkspaceDialog
    : public QDialog
{
  Q_OBJECT

  QPalette defaultPal_;
  QString lastValidLocalWorkDirSetting_;
  std::unique_ptr<insight::RemoteLocation> remoteLocation_;
  bool lockRemoteExecution_;

  /**
   * reachability of the servers in the host combo box,
   * determined in background. Missing entry: not yet known.
   */
  std::map<QString,bool> hostOnline_;

  /**
   * remote directory created by the "new" button.
   * Removed again, if not used in the end.
   */
  struct CreatedRemoteDir
  {
    insight::RemoteServer::ConfigPtr server;
    boost::filesystem::path directory;
  };
  std::unique_ptr<CreatedRemoteDir> createdRemoteDir_;

  // checks
  bool isTemporaryLocalDirectorySelected() const;
  bool isValidWorkingDir(const boost::filesystem::path& newDirPath) const;

  // display update
  void displayCurrentServerName();
  void displayCurrentRemoteWorkingDir();
  void setHostState(const QString& serverLabel, bool online);
  void updateCreateRemoteDirButton();

  // change configuration
  void changeWorkingDirectory(const QString& newDir);
  void checkAndChangeRemoteConfig(const QString& serverName, const QString& remoteDir);
  void setRemoteConfigFromWorkingDir();
  void createRemoteDirectory();
  void removeCreatedRemoteDirIfUnused(bool accepted);

public:
  explicit QExecutionWorkspaceDialog(
      const insight::CaseDirectory* localDirectory,
      const insight::RemoteLocation* remoteLocation,
      QWidget *parent = nullptr );
  ~QExecutionWorkspaceDialog();

  void lockRemoteExecution(const QString& reason);

  // query
  boost::filesystem::path localDirectory() const;
  insight::RemoteLocation* remoteLocation() const;

private:
  Ui::QExecutionWorkspaceDialog *ui;
};

#endif // QEXECUTIONENVIRONMENTDIALOG_H
