#ifndef REMOTEPARAVIEW_H
#define REMOTEPARAVIEW_H

#include <QDialog>

#include "base/boost_include.h"

#include "openfoam/remoteexecution.h"

namespace Ui {
class RemoteParaview;
}

class RemoteParaview : public QDialog
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
