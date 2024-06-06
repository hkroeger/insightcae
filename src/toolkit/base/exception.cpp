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


#include "exception.h"
#include <sstream>
#include <cstdlib>
#include <thread>

#include <dlfcn.h>    // for dladdr
#include <cxxabi.h>   // for __cxa_demangle
#include <cstdio>
#include <cstdlib>

#include "stdarg.h"

#include "boost/stacktrace.hpp"
#include "boost/format.hpp"
#include "boost/algorithm/string.hpp"

#define DEBUG

using namespace std;

namespace insight
{




std::ostream& operator<<(std::ostream& os, const Exception& ex)
{
  os<<static_cast<std::string>(ex);
  return os;
}




std::string splitMessage(
    const std::string& message,
    std::size_t width,
    std::string begMark,
    string endMark,
    std::string whitespace
    )
{

  if (!begMark.empty())
  {
    width-=2;
    begMark+=" ";
  }

  if (!endMark.empty())
  {
    width-=2;
    endMark=" "+endMark;
  }

  std::string source(message);
  std::vector<string> splittext;

  while ( source.length()>0 )
  {

    size_t i = string::npos;

    std::vector<size_t> splitPoints;
    do
    {
      size_t inl = source.find_first_of("\n", i==string::npos? 0 : i+1 );
      i = source.find_first_of(whitespace, i==string::npos? 0 : i+1 );

      if (i == string::npos)
      {
        i = source.length();
      }
      splitPoints.push_back(i);


      if ( (inl != string::npos) && (i != string::npos) && (inl < i) )
      {
        i = inl;
        splitPoints.back()=inl;
        break;
      }

      if (i == string::npos)
        break;

    }
    while ( (i<width) && (i<source.length()) );

    // try to go back, if too wide
    if (i>width)
    {
      if (splitPoints.size()>1)
      {
        i = *(++splitPoints.rbegin());
      }
    }

    if (i>0)
    {
      splittext.push_back( source.substr(0, i) ); // without white space
      source.erase(0, i+1); // remove including whitespace
    }
    else
      break;

  }

  std::string result;
  for (const auto& l: splittext)
  {
    result+=begMark;
    result+=l;
    if (l.size()<width)
    {
      result+=string(width-l.size(), ' ');
    }
    result+=endMark+"\n";
  }

  return result;
}




void Exception::saveContext(bool strace)
{
  std::vector<std::string> context_list;
  ExceptionContext::getCurrent().snapshot(context_list);

  if (context_list.size()>0)
  {
    context_="The problem occurred";
    for (const std::string& c: context_list)
      {
        context_+= "\nwhile "+c;
      }
  }

  if (strace)
  {
    ostringstream trace_buf;
    trace_buf  << boost::stacktrace::stacktrace();
    strace_=trace_buf.str();
  }
  else
    strace_="";
}




Exception::Exception()
{
  saveContext(true);
}




Exception::Exception(std::string fmt, ...)
{
    char str[5000];
    va_list args;
    va_start(args, fmt);
    vsnprintf(str, sizeof(str), fmt.c_str(), args);
    va_end(args);
    int l = strlen(str); if(str[l-1] == '\n') str[l-1] = '\0';

    message_=str;
    saveContext(true);

    dbg(2)<<message_<<"\n"<<context_<<std::endl;
}




// Exception::Exception(const std::string& msg, const std::string& strace)
//   : message_(msg), strace_(strace)
// {
//   dbg(2)<<msg<<std::endl;
// }




Exception::operator std::string() const
{
  return message();
}


const std::string& Exception::description() const
{
    return message_;
}

const std::string& Exception::context() const
{
    return context_;
}



string Exception::message() const
{
   return message_+"\n"+context_;
}




const char* Exception::what() const noexcept
{
   whatMessage_=message();
   return whatMessage_.c_str();
}


IntendedBreak::IntendedBreak(const std::string& msg)
    : Exception(msg)
{}



const std::map<string, cad::FeaturePtr> &CADException::contextGeometry() const
{
  return contextGeometry_;
}




void assertion(bool condition, std::string fmt, ...)
{
  if (!condition)
  {
      char str[5000];
      va_list args;
      va_start(args, fmt);
      vsnprintf(str, sizeof(str), fmt.c_str(), args);
      va_end(args);
      int l = strlen(str); if(str[l-1] == '\n') str[l-1] = '\0';

      throw insight::Exception(
                  std::string("Internal error: condition violated: ")
                  + str );
  }
}




CurrentExceptionContext::CurrentExceptionContext(int verbosityLevel, std::string msgFmt, ...)
  : verbosityLevel_(verbosityLevel)
{
  char s[5000];
  va_list args;
  va_start(args, msgFmt);
  vsnprintf(s, sizeof(s), msgFmt.c_str(), args);
  va_end(args);
  int l = strlen(s); if(s[l-1] == '\n') s[l-1] = '\0';

  start(s);
}




CurrentExceptionContext::CurrentExceptionContext(std::string msgFmt, ...)
    : verbosityLevel_(1)
{
  char s[5000];
  va_list args;
  va_start(args, msgFmt);
  vsnprintf(s, sizeof(s), msgFmt.c_str(), args);
  va_end(args);
  int l = strlen(s); if(s[l-1] == '\n') s[l-1] = '\0';

  start(s);
}




void CurrentExceptionContext::start(const char* msg)
{
  this->std::string::operator=(msg);

  if (const char* iv = getenv("INSIGHT_VERBOSE"))
  {
      if (atoi(iv)>=verbosityLevel_)
      {
        std::cout << ">> [BEGIN, "<< std::this_thread::get_id() <<"] " << contextDescription() << std::endl;
      }
  }
  ExceptionContext::getCurrent().push_back(this);
}




CurrentExceptionContext::~CurrentExceptionContext()
{
  if (ExceptionContext::getCurrent().back()==this)
    ExceptionContext::getCurrent().pop_back();
  else
    {
      std::cerr<<"Oops: CurrentExceptionContext destructor: expected to be last!"<<endl;
    }

  if (const char* iv = getenv("INSIGHT_VERBOSE"))
  {
      if (atoi(iv)>=verbosityLevel_)
      {
        std::cout << "<< [FINISH, "<< std::this_thread::get_id() <<"]: "<<contextDescription() << std::endl;
      }
  }
}




std::string CurrentExceptionContext::contextDescription() const
{
  return *this;
}




class NullBuffer : public std::streambuf
{
public:
  int overflow(int c) { return c; }
};




std::ostream& dbg(int verbosityLevel)
{
  static NullBuffer nullBuffer;
  static std::ostream nullOstream(&nullBuffer);

  if (getenv("INSIGHT_VERBOSE"))
  {
      int vl = atoi(getenv("INSIGHT_VERBOSE"));
      if (vl>=verbosityLevel)
      {
        std::cerr<<"[DBG, " << std::this_thread::get_id() <<"]: ";
        return std::cerr;
      }
  }

  return nullOstream;
}




void ExceptionContext::snapshot(std::vector<std::string>& context)
{
  context.clear();
  for (const auto& i: *this)
    {
      context.push_back( i->contextDescription() );
    }
}




ExceptionContext& ExceptionContext::getCurrent()
{
  static thread_local ExceptionContext thisThreadsExceptionContext;
  return thisThreadsExceptionContext;
}




std::string valueList_to_string(const arma::mat& vals, arma::uword maxlen)
{
  std::ostringstream os;
  os <<"(";

  arma::uword nr=vals.n_rows;
  if (nr>0)
  {
    size_t n1=std::min(nr, maxlen-2);

    for (size_t i=0; i<n1; i++)
      os<<" "<<vals.row(i);

    if (n1<vals.size())
      {
        os << " .... "<<vals.row(nr-1);
      }
  }

  os<<" )";
  return os.str();
}




WarningDispatcher::WarningDispatcher()
  : superDispatcher_(nullptr)
{}

void WarningDispatcher::setSuperDispatcher(WarningDispatcher *superDispatcher)
{
//#if !(defined(WIN32)&&defined(DEBUG))
  superDispatcher_=superDispatcher;
//#endif
}

void WarningDispatcher::issue(const std::string& message)
{
  issue(insight::Exception(message));
}

void WarningDispatcher::issue(const insight::Exception& warning)
{
  if (superDispatcher_)
  {
    superDispatcher_->issue(warning);
  }
  else
  {
    displayFramed("Warning follows", warning, '-', std::cerr);
    warnings_.push_back(warning);
  }
}

void displayFramed(const std::string& title, const std::string& msg, char titleChar, ostream &os)
{
  int dif=80-title.size()-2-2;
  int nx=dif/2;
  int ny=dif-nx;

  os
     <<"\n"
       "+"<<std::string(nx,titleChar)<<" "<<title<<" "<<std::string(ny, titleChar)<<"+\n"
     <<"|"      <<string(78, ' ')                                                   <<"|\n"
                            <<splitMessage(msg, 80, "|", "|")
     <<"|"      <<string(78, ' ')                                                   <<"|\n"<<
       "+------------------------------------------------------------------------------+\n"
     <<"\n"
       ;

}

const decltype(WarningDispatcher::warnings_)& WarningDispatcher::warnings() const
{
  return warnings_;
}

size_t WarningDispatcher::nWarnings() const
{
  return warnings_.size();
}


////#if !(defined(WIN32)&&defined(DEBUG))
//thread_local
////#endif
//WarningDispatcher thisThreadsWarnings;

WarningDispatcher& WarningDispatcher::getCurrent()
{
  static thread_local WarningDispatcher thisThreadsWarnings;
  return thisThreadsWarnings;
}



void Warning(std::string msgFmt, ...)
{
  char msg[5000];
  va_list args;
  va_start(args, msgFmt);
  vsnprintf(msg, sizeof(msg), msgFmt.c_str(), args);
  va_end(args);
  int l = strlen(msg); if(msg[l-1] == '\n') msg[l-1] = '\0';

  WarningDispatcher::getCurrent().issue( msg );
}


void Warning(const std::exception& ex)
{
    Warning(ex.what());
}

void UnhandledExceptionHandling::handler()
{
  std::cerr<<"Unhandled exception occurred!"<<std::endl;
  std::cerr << boost::stacktrace::stacktrace();
  exit(1);
}

UnhandledExceptionHandling::UnhandledExceptionHandling()
{
    std::set_terminate( handler );
}





void printException(const std::exception& e)
{
  std::ostringstream title;
  title<<"*** ERROR ["<< std::this_thread::get_id() <<"] ***";

  if (const auto* ie = dynamic_cast<const insight::Exception*>(&e))
  {
    displayFramed(title.str(), ie->message(), '=', std::cerr);
    if (getenv("INSIGHT_STACKTRACE"))
    {
      std::cerr << "Stack trace:" << std::endl
                << ie->strace() <<std::endl;
    }
  }
  else
  {
      displayFramed(title.str(), e.what(), '=', std::cerr);
  }
}

string vector_to_string(const arma::mat &vals, bool addMag)
{
  std::ostringstream os;
  os <<"(";
  for (arma::uword i=0; i<vals.n_elem; i++)
  {
    os<<" "<<vals(i);
  }
  os<<" )";
  if (addMag)
  {
    os<<" |"<<arma::norm(vals,2)<<"|";
  }
  return os.str();
}




ExternalProcessFailed::ExternalProcessFailed()
    : retcode_(0)
{}

ExternalProcessFailed::ExternalProcessFailed(
    int retcode,
    const std::string &exename,
    const std::string &errout )

  : Exception(
        str(boost::format(
                "Execution of external application \"%s\" failed with return code %d!\n")
            % exename % retcode),
        true),
    retcode_(retcode),
    exename_(exename),
    errout_(errout)
{}


string ExternalProcessFailed::message() const
{
  return Exception::message()
         + ( errout_.size()>0 ?
                ( "Error output was:\n " + errout_ + "\n" )
                               :
                ( "There was no error output." )
            );
}

const string &ExternalProcessFailed::exeName() const
{
  return exename_;
}




UnsupportedFeature::UnsupportedFeature()
{}

UnsupportedFeature::UnsupportedFeature(const string &msg, bool strace)
    : Exception(msg, strace)
{}



UnhandledSelection::UnhandledSelection(const std::string &contextMsg)
    : Exception(
        "internal error: unhandled selection" +
        ((!contextMsg.empty())?(" in "+contextMsg):std::string())
    )
{}











}
