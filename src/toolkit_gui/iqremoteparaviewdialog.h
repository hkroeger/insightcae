#ifndef REMOTEPARAVIEW_H
#define REMOTEPARAVIEW_H

#include "toolkit_gui_export.h"


#include <QDialog>

#include "base/boost_include.h"
#include "base/remoteexecution.h"
#include "remoteparaview.h"

namespace Ui {
class IQRemoteParaviewDialog;
}

class TOOLKIT_GUI_EXPORT IQRemoteParaviewDialog : public QDialog
{
  Q_OBJECT

  insight::RemoteExecutionConfig rec_;
  std::shared_ptr<insight::RemoteParaview> rp_;

public:
  explicit IQRemoteParaviewDialog(
      const insight::RemoteExecutionConfig& rec,
      QWidget *parent = nullptr
      );
  ~IQRemoteParaviewDialog();

  std::shared_ptr<insight::RemoteParaview> remoteParaviewProcess();

private:
  Ui::IQRemoteParaviewDialog *ui;
};

#endif // REMOTEPARAVIEW_H
