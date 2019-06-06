#ifndef REMOTEPARAVIEW_H
#define REMOTEPARAVIEW_H

#include <QDialog>

namespace Ui {
class RemoteParaview;
}

class RemoteParaview : public QDialog
{
  Q_OBJECT

public:
  explicit RemoteParaview(QWidget *parent = nullptr);
  ~RemoteParaview();

  Ui::RemoteParaview *ui;
};

#endif // REMOTEPARAVIEW_H
