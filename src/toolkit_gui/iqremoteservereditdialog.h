#ifndef IQREMOTESERVEREDITDIALOG_H
#define IQREMOTESERVEREDITDIALOG_H

#include "toolkit_gui_export.h"

#include "iqremoteserverlistmodel.h"

#include <QDialog>

namespace Ui {
class IQRemoteServerEditDialog;
}

class TOOLKIT_GUI_EXPORT IQRemoteServerEditDialog
    : public QDialog
{
  Q_OBJECT

  IQRemoteServerListModel serverListModel_;

  /**
   * enable launch/shutdown according to the state of the current server
   */
  void updateServerActionButtons();

public:
  explicit IQRemoteServerEditDialog(QWidget *parent = nullptr);
  ~IQRemoteServerEditDialog();

  void accept() override;

private:
  Ui::IQRemoteServerEditDialog *ui;
};

#endif // IQREMOTESERVEREDITDIALOG_H
