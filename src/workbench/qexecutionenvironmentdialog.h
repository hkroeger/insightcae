#ifndef QEXECUTIONENVIRONMENTDIALOG_H
#define QEXECUTIONENVIRONMENTDIALOG_H

#include <QDialog>

#include "base/tools.h"
#include "base/remoteexecution.h"

namespace Ui {
class QExecutionEnvironmentDialog;
}

class QExecutionEnvironmentDialog
    : public QDialog
{
  Q_OBJECT

  QPalette defaultPal_;
  QString lastValidLocalWorkDirSetting_;
  std::unique_ptr<insight::RemoteLocation> remoteLocation_;

  void resetServerName();
  void resetRemoteWorkingDir();
  void setInvalidWorkingDir(bool changeLE=true);
  bool checkAndUpdateWorkingDir(const QString& newDir, bool changeLE=true);
  bool checkAndUpdateRemoteConfig(const QString& serverName, const QString& remoteDir);
  void setRemoteConfigFromWorkingDir();

public:
  explicit QExecutionEnvironmentDialog(
      const insight::CaseDirectory* localDirectory,
      const insight::RemoteLocation* remoteLocation,
      QWidget *parent = nullptr );
  ~QExecutionEnvironmentDialog();

  boost::filesystem::path localDirectory() const;
  insight::RemoteLocation* remoteLocation() const;

private:
  Ui::QExecutionEnvironmentDialog *ui;
};

#endif // QEXECUTIONENVIRONMENTDIALOG_H
