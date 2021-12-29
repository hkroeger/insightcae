#ifndef IQDEBUGSTREAM_H
#define IQDEBUGSTREAM_H

#include "toolkit_gui_export.h"


#include <iostream>
#include <string>

#include <QTextEdit>
#include <qapplication.h>

#include "base/streamredirector.h"


class TOOLKIT_GUI_EXPORT IQDebugStream
: public QObject, 
  public insight::StreamRedirector
{
    Q_OBJECT

protected:
    void processLine(std::string line) override;

public:
    IQDebugStream(std::ostream &stream);

    static void registerQDebugMessageHandler();
    
private:
    static void myQDebugMessageHandler(QtMsgType, const QMessageLogContext&, const QString& msg);
    
Q_SIGNALS:
    void appendText(const QString& text);

};


#endif // Q_DEBUGSTREAM_H
