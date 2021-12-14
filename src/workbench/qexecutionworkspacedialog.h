#ifndef QEXECUTIONENVIRONMENTDIALOG_H
#define QEXECUTIONENVIRONMENTDIALOG_H

#include <QDialog>

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

  // checks
  bool isTemporaryLocalDirectorySelected() const;
  bool isValidWorkingDir(const boost::filesystem::path& newDirPath) const;

  // display update
  void displayCurrentServerName();
  void displayCurrentRemoteWorkingDir();

  // change configuration
  void changeWorkingDirectory(const QString& newDir);
  void checkAndChangeRemoteConfig(const QString& serverName, const QString& remoteDir);
  void setRemoteConfigFromWorkingDir();

public:
  explicit QExecutionWorkspaceDialog(
      const insight::CaseDirectory* localDirectory,
      const insight::RemoteLocation* remoteLocation,
      QWidget *parent = nullptr );
  ~QExecutionWorkspaceDialog();

  // query
  boost::filesystem::path localDirectory() const;
  insight::RemoteLocation* remoteLocation() const;

private:
  Ui::QExecutionWorkspaceDialog *ui;
};

#endif // QEXECUTIONENVIRONMENTDIALOG_H
