/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */


#include "qdebugstream.h"


QMutex qdebugstream_mutex;


Q_DebugStream::Q_DebugStream ( std::ostream &stream )
  : m_stream ( stream )
{
  m_old_buf = stream.rdbuf();
  stream.rdbuf ( this );
}

Q_DebugStream::~Q_DebugStream()
{
  m_stream.rdbuf ( m_old_buf );
}

void Q_DebugStream::registerQDebugMessageHandler()
{
  qInstallMessageHandler ( myQDebugMessageHandler );
}


void Q_DebugStream::myQDebugMessageHandler ( QtMsgType, const QMessageLogContext&, const QString& msg )
{
  std::cout << msg.toStdString();
}

void Q_DebugStream::sendCurLine()
{
  QString cl=QString ( curline_.c_str() );
  curline_="";
  emit appendText ( cl );
}


//This is called when a std::endl has been inserted into the stream
Q_DebugStream::int_type Q_DebugStream::overflow ( Q_DebugStream::int_type v )
{
  qdebugstream_mutex.lock();
  if ( v == '\n' )
    {
      sendCurLine();
    }
  qdebugstream_mutex.unlock();
  return v;
}


std::streamsize Q_DebugStream::xsputn ( const char *p, std::streamsize n )
{
  qdebugstream_mutex.lock();
  std::string str ( p, n );

  if ( boost::find_first ( str, "\n" ) )
    {
      std::vector<std::string> strSplitted;
      boost::split ( strSplitted, str, boost::is_any_of ( "\n" ) );

      curline_+=strSplitted[0];
      sendCurLine();

      for ( int i = 1; i < strSplitted.size()-1; i++ )
        {
          curline_=strSplitted[i];
          sendCurLine();
        }

      if ( strSplitted.size() >1 )
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
