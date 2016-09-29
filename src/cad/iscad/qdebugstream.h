#ifndef Q_DEBUGSTREAM_H
#define Q_DEBUGSTREAM_H

#include <iostream>
#include <streambuf>
#include <string>

#include <QTextEdit>
#include <QMutex>
#include <qapplication.h>

extern QMutex qdebugstream_mutex;

class Q_DebugStream 
: public QObject, public std::basic_streambuf<char>
{
    Q_OBJECT
public:
    Q_DebugStream(std::ostream &stream/*, QTextEdit* text_edit*/) : m_stream(stream)
    {
//         log_window = text_edit;
        m_old_buf = stream.rdbuf();
        stream.rdbuf(this);
    }

    ~Q_DebugStream()
    {
        m_stream.rdbuf(m_old_buf);
    }

    static void registerQDebugMessageHandler()
    {
        qInstallMsgHandler(myQDebugMessageHandler);
//         qInstallMessageHandler(myQDebugMessageHandler);
    }

private:

    static void myQDebugMessageHandler(QtMsgType, const char *msg)
    {
        std::cout << msg;
    }

protected:

    //This is called when a std::endl has been inserted into the stream
    virtual int_type overflow(int_type v)
    {
        qdebugstream_mutex.lock();
        if (v == '\n')
        {
            //log_window->append("");
            emit appendText("");
        }
        qdebugstream_mutex.unlock();
        return v;
    }


    virtual std::streamsize xsputn(const char *p, std::streamsize n)
    {
        qdebugstream_mutex.lock();
        QString str(p);
        if(str.contains("\n")){
            QStringList strSplitted = str.split("\n");

//             log_window->moveCursor (QTextCursor::End);
//             log_window->insertPlainText (strSplitted.at(0)); //Index 0 is still on the same old line
            emit appendText(strSplitted.at(0));

            for(int i = 1; i < strSplitted.size(); i++){
                //log_window->append(strSplitted.at(i));
                emit appendText(strSplitted.at(i));
            }
        }else{
            emit appendText(str);
//             log_window->moveCursor (QTextCursor::End);
//             log_window->insertPlainText (str);
        }
        qdebugstream_mutex.unlock();
        return n;
    }
    
signals:
    void appendText(const QString& text);

private:
    std::ostream &m_stream;
    std::streambuf *m_old_buf;
//     QTextEdit* log_window;
};


#endif // Q_DEBUGSTREAM_H
