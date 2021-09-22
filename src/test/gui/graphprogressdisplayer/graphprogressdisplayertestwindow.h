#ifndef GRAPHPROGRESSDISPLAYERTESTWINDOW_H
#define GRAPHPROGRESSDISPLAYERTESTWINDOW_H

#include <QMainWindow>

namespace Ui {
class GraphProgressDisplayerTestWindow;
}

class GraphProgressDisplayerTestWindow : public QMainWindow
{
  Q_OBJECT

  size_t iter_=0;

public:
  explicit GraphProgressDisplayerTestWindow(QWidget *parent = nullptr);
  ~GraphProgressDisplayerTestWindow();

private:
  Ui::GraphProgressDisplayerTestWindow *ui;
};

#endif // GRAPHPROGRESSDISPLAYERTESTWINDOW_H
