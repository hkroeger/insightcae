#ifndef REMOTEPARAVIEW_H
#define REMOTEPARAVIEW_H

#include "toolkit_gui_export.h"


#include <QDialog>

#include "base/boost_include.h"
#include "base/remoteexecution.h"

namespace Ui {
class RemoteParaview;
}

class TOOLKIT_GUI_EXPORT RemoteParaview : public QDialog
{
  Q_OBJECT

  insight::RemoteExecutionConfig rec_;

public:
  explicit RemoteParaview(
      const insight::RemoteExecutionConfig& rec,
      QWidget *parent = nullptr
      );
  ~RemoteParaview();

  Ui::RemoteParaview *ui;
};

#endif // REMOTEPARAVIEW_H
