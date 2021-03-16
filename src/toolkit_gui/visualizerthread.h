#ifndef VISUALIZERTHREAD_H
#define VISUALIZERTHREAD_H

#include <QThread>

class ParameterSetDisplay;
namespace insight { class ProgressDisplayer; }

class VisualizerThread
    : public QThread
{
  Q_OBJECT

  ParameterSetDisplay *psd_;

  void run() override;

public:
  VisualizerThread(ParameterSetDisplay* psd);
};
#endif // VISUALIZERTHREAD_H
