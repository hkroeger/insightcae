#ifndef LOGVIEWERWIDGET_H
#define LOGVIEWERWIDGET_H

#include "toolkit_gui_export.h"
#include "base/progressdisplayer.h"

#include <QPlainTextEdit>


class TOOLKIT_GUI_EXPORT LogViewerWidget
: public QPlainTextEdit,
  public insight::ProgressDisplayer
{
public:
  LogViewerWidget(QWidget* parent=nullptr);

  void update ( const insight::ProgressState& pi ) override;
  void setActionProgressValue(const std::string &path, double value) override;
  void setMessageText(const std::string &path, const std::string& message) override;
  void finishActionProgress(const std::string &path) override;
  void reset() override;

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
