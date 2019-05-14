#ifndef LOGVIEWERWIDGET_H
#define LOGVIEWERWIDGET_H

#include <QPlainTextEdit>

class LogViewerWidget
: public QPlainTextEdit
{
public:
  LogViewerWidget(QWidget* parent=nullptr);

public Q_SLOTS:
  void appendLine(const QString& line);
  void saveLog();
  void sendLog();
  void clearLog();
  void autoScrollLog();

};

#endif // LOGVIEWERWIDGET_H
