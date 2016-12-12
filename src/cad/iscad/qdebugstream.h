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
    Q_DebugStream(std::ostream &stream) 
    : m_stream(stream)
    {
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
    
    void sendCurLine()
    {
      QString cl=QString(curline_.c_str());
      curline_="";
      emit appendText(cl);
    }

protected:

    //This is called when a std::endl has been inserted into the stream
    virtual int_type overflow(int_type v)
    {
        qdebugstream_mutex.lock();
        if (v == '\n')
        {
            //log_window->append("");
            sendCurLine();
        }
        qdebugstream_mutex.unlock();
        return v;
    }


    virtual std::streamsize xsputn(const char *p, std::streamsize n)
    {
        qdebugstream_mutex.lock();
	std::string str(p, n);

// 	std::cerr<<int(str[0])<<" >>"<<str<<"<<"<<int(str[str.size()-1])<<std::endl;
        if (boost::find_first(str, "\n"))
	{
            std::vector<std::string> strSplitted;
	    boost::split(strSplitted, str, boost::is_any_of("\n"));
	    
//             for(int i = 0; i < strSplitted.size(); i++)
// 	    {
// 	      std::cerr<<"i="<<i<<" : >>"<<strSplitted[i]<<"<<"<<std::endl;
// 	    }

	    curline_+=strSplitted[0];
            sendCurLine();

            for(int i = 1; i < strSplitted.size()-1; i++)
	    {
	      curline_=strSplitted[i];
	      sendCurLine();
            }
            
            if (strSplitted.size()>1)
	    {
	      curline_+=strSplitted[strSplitted.size()-1];
	    }
        }
        else
	{
	      curline_+=str;
        }
        qdebugstream_mutex.unlock();
        return n;
    }
    
signals:
    void appendText(const QString& text);

};


#endif // Q_DEBUGSTREAM_H
