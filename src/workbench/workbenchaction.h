#ifndef WORKBENCHACTION_H
#define WORKBENCHACTION_H

#include <QObject>

#include "base/exception.h"
#include "base/resultset.h"
#include "base/progressdisplayer.h"

class AnalysisForm;

class WorkbenchAction
    : public QObject
{
  Q_OBJECT

protected:
  AnalysisForm* af_;

  void exceptionEmitter();

public:
  WorkbenchAction(AnalysisForm* af);
  virtual ~WorkbenchAction();

public Q_SLOTS:
  virtual void onCancel() =0;

Q_SIGNALS:
  void analysisProgressUpdate(const insight::ProgressState& ps);
  void logMessage(const QString& logmsg);

  void killed();
  void finished(insight::ResultSetPtr results);
  void failed(insight::Exception e);
  void warning(insight::Exception e);
};

#endif // WORKBENCHACTION_H
