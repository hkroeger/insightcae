#ifndef LOGVIEWERWIDGET_H
#define LOGVIEWERWIDGET_H

#include "toolkit_gui_export.h"


#include <QPlainTextEdit>

namespace insight {
class ProgressState;
}

class TOOLKIT_GUI_EXPORT LogViewerWidget
: public QPlainTextEdit
{
public:
  LogViewerWidget(QWidget* parent=nullptr);

public Q_SLOTS:
  void appendLine(const QString& line);
  void appendErrorLine(const QString& line);
  void appendDimmedLine(const QString& line);
  void appendLogMessage(const insight::ProgressState& ps);
  void saveLog();
  void sendLog();
  void clearLog();
  void autoScrollLog();

};

#endif // LOGVIEWERWIDGET_H
