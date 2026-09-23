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
#include <boost/format/format_fwd.hpp>
#include <exception>
#include <ostream>
#include <sstream>
#include <cstdlib>
#include <thread>

#ifdef WIN32
#include <windows.h>  // FlsAlloc / FlsGetValue / FlsSetValue
#endif

#include <dlfcn.h>    // for dladdr
#include <cxxabi.h>   // for __cxa_demangle
#include <cstdio>
#include <cstdlib>

#include "boost/lexical_cast.hpp"


#include "boost/stacktrace.hpp"
#include "boost/format.hpp"
#include "boost/algorithm/string.hpp"
#include "base/translations.h"

#define DEBUG

using namespace std;

namespace insight
{




std::ostream& operator<<(std::ostream& os, const ExceptionBase& ex)
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




ErrorDescription::ErrorDescription(const std::string &msg)
  : std::string(msg),
    thread_id_(boost::lexical_cast<std::string>(
        std::this_thread::get_id()))
{}

ErrorDescription::~ErrorDescription()
{}




void ExceptionBase::saveContext(bool strace)
{
  std::vector<std::string> context_list;
  ExceptionContext::getCurrent().snapshot(context_list);

  if (context_list.size()>0)
  {
    errorDescription_->context_=_("The problem occurred");
    for (const std::string& c: context_list)
      {
        errorDescription_->context_+= std::string("\n") + _("while") + " " + c;
      }
  }

  errorDescription_->strace_="";

  if (const char* iv = getenv("INSIGHT_VERBOSE"))
  {
      if (strace && (atoi(iv)>=VerbosityLevel::DeepDetail) )
      {
        ostringstream trace_buf;
        trace_buf  << boost::stacktrace::stacktrace();
        errorDescription_->strace_=trace_buf.str();
      }
  }

}






// Exception::Exception(const std::string& msg, const std::string& strace)
//   : message_(msg), strace_(strace)
// {
//   dbg(2)<<msg<<std::endl;
// }





const std::string& ExceptionBase::message() const
{
    return *errorDescription_;
}


const std::string& ExceptionBase::context() const
{
    return errorDescription_->context_;
}


const std::string& ExceptionBase::strace() const
{
    return errorDescription_->strace_;
}


ErrorDescriptionPtr ExceptionBase::description() const
{
    return errorDescription_;
}


ExceptionBase::operator std::string() const
{
    return message();
}

const char* ExceptionBase::what() const noexcept
{
   whatMessage_=message();
   return whatMessage_.c_str();
}


IntendedBreak::IntendedBreak(const std::string& msg)
    : Exception(msg)
{}





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

      throw insight::Exception( str );
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



int requestedVerbosityLevel()
{
    int vl=0;
    if (auto *verbev=getenv("INSIGHT_VERBOSE"))
    {
        vl = atoi(verbev);
    }
    return vl;
}


std::ostream& dbg(int verbosityLevel)
{
  static NullBuffer nullBuffer;
  static std::ostream nullOstream(&nullBuffer);

  if (requestedVerbosityLevel()>=verbosityLevel)
  {
    std::cerr<<"[DBG, " << std::this_thread::get_id() <<"]: ";
    return std::cerr;
  }

  return nullOstream;
}


ostream &dbg_slot(const std::string &signalName)
{
    ostream& os=dbg(DeepDetail);
    os << "handling signal " << signalName << ".\n";
    return os;
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
#ifdef WIN32
  // thread_local with non-trivial destructor in a DLL causes crashes on
  // Windows/MinGW because __cxa_thread_atexit fires after the DLL is unmapped.
  // Use FlsAlloc so the destructor callback runs during DLL_THREAD_DETACH /
  // DLL_PROCESS_DETACH, while the DLL is still mapped — no crash, no leak.
  static DWORD flsIdx = FlsAlloc([](PVOID ptr) {
      delete static_cast<ExceptionContext*>(ptr);
  });
  auto* p = static_cast<ExceptionContext*>(FlsGetValue(flsIdx));
  if (!p) {
      p = new ExceptionContext();
      FlsSetValue(flsIdx, p);
  }
  return *p;
#else
  static thread_local ExceptionContext thisThreadsExceptionContext;
  return thisThreadsExceptionContext;
#endif
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
            _("Execution of external application \"%s\" failed with return code %d!") )
            % exename % retcode),
        true),
    retcode_(retcode),
    exename_(exename),
    errout_(errout)
{
    errorDescription_->errorDetails_=
        ( errout_.size()>0 ?
            ( std::string(_("Error output was:")) + "\n "+errout_+"\n" )
                            :
            ( _("There was no error output.") )
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


ElementNotFoundException::ElementNotFoundException(
    const std::string &msg)
    : Exception(msg)
{}




}
