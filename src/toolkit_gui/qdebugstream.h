#ifndef Q_DEBUGSTREAM_H
#define Q_DEBUGSTREAM_H

#include <iostream>
#include <streambuf>
#include <string>

#include <QTextEdit>
#include <QMutex>
#include <qapplication.h>

#include "base/boost_include.h"


extern QMutex qdebugstream_mutex;


class Q_DebugStream 
: public QObject, 
  public std::basic_streambuf<char>
{
  
    Q_OBJECT
    
private:
    std::ostream &m_stream;
    std::streambuf *m_old_buf;
    std::string curline_;
    
    
public:
    Q_DebugStream(std::ostream &stream);
    ~Q_DebugStream();
    static void registerQDebugMessageHandler();
    
private:
    static void myQDebugMessageHandler(QtMsgType, const char *msg);
    void sendCurLine();

protected:

    //This is called when a std::endl has been inserted into the stream
    virtual int_type overflow(int_type v);
    virtual std::streamsize xsputn(const char *p, std::streamsize n);
    
signals:
    void appendText(const QString& text);

};


#endif // Q_DEBUGSTREAM_H
