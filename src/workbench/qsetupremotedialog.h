#ifndef QSETUPREMOTEDIALOG_H
#define QSETUPREMOTEDIALOG_H

#include <QDialog>

namespace Ui {
class QSetupRemoteDialog;
}

class QSetupRemoteDialog : public QDialog
{
  Q_OBJECT

public:
  explicit QSetupRemoteDialog(const QString& hostName, const QString& path, QWidget *parent = nullptr);
  ~QSetupRemoteDialog();

    QString hostName() const;
    QString remoteDirectory() const;

private:
  Ui::QSetupRemoteDialog *ui;
};

#endif // QSETUPREMOTEDIALOG_H
