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

#include "boost/stacktrace.hpp"
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


std::string splitMessage(const std::string& message, std::size_t width, std::string begMark, string endMark, std::string whitespace)
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

  std::size_t  currIndex = width - 1;
  std::size_t  sizeToElim;
  while ( currIndex < message.length() )
  {
    currIndex = source.find_last_of(whitespace,currIndex + 1);
    if (currIndex == std::string::npos)
        break;
    currIndex = source.find_last_not_of(whitespace,currIndex);
    if (currIndex == std::string::npos)
        break;
    sizeToElim = source.find_first_not_of(whitespace,currIndex + 1) - currIndex - 1;

    source.replace( currIndex + 1, sizeToElim , "\n");
    currIndex += (width + 1); //due to the recently inserted "\n"
  }

  std::vector<string> splittext;
  boost::split(splittext, source, boost::is_any_of("\n"));

  string result;
  for (const auto& l: splittext)
  {
    result+=begMark+l;
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
    string context="\nThe problem occurred";
    for (const std::string& c: context_list)
      {
        context+= "\nwhile "+c;
      }
    message_ += "\n" + context;
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

Exception::Exception(const std::string& msg, bool strace)
  : message_(msg)
{
  saveContext(strace);
}

Exception::Exception(const string &msg, const std::map<string, cad::FeaturePtr> &contextGeometry, bool strace)
  : message_(msg),
    contextGeometry_(contextGeometry)
{
  saveContext(strace);
}

Exception::Exception(const std::string& msg, const std::string& strace)
  : message_(msg), strace_(strace)
{
}

Exception::operator std::string() const
{
  return message_;
}


const char* Exception::what() const noexcept
{
  return message_.c_str();
}

const std::map<string, cad::FeaturePtr> &Exception::contextGeometry() const
{
  return contextGeometry_;
}

void assertion(bool condition, const std::string& context_message)
{
  if (!condition)
    throw insight::Exception("Internal error: condition violated: "+context_message);
}


CurrentExceptionContext::CurrentExceptionContext(const std::string& desc, bool verbose)
: desc_(desc)
{
  if (getenv("INSIGHT_VERBOSE"))
  {
    if (verbose)
    {
      std::cout << ">> [BEGIN, "<< std::this_thread::get_id() <<"] " << desc << std::endl;
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

  if (getenv("INSIGHT_VERBOSE"))
  {
    std::cout << "<< [FINISH, "<< std::this_thread::get_id() <<"]: "<<desc_ << std::endl;
  }
}

std::string CurrentExceptionContext::contextDescription() const
{
  return desc_;
}

CurrentExceptionContext::operator std::string() const
{
  return desc_;
}



class NullBuffer : public std::streambuf
{
public:
  int overflow(int c) { return c; }
};


std::ostream& dbg()
{
  static NullBuffer nullBuffer;
  static std::ostream nullOstream(&nullBuffer);

  if (getenv("INSIGHT_VERBOSE"))
  {
    std::cerr<<"[DBG, " << std::this_thread::get_id() <<"]: ";
    return std::cerr;
  }
  else
  {
    return nullOstream;
  }
}



void ExceptionContext::snapshot(std::vector<std::string>& context)
{
  context.clear();
  for (const auto& i: *this)
    {
      context.push_back( i->contextDescription() );
    }
}

////#if !(defined(WIN32)&&defined(DEBUG))
//thread_local
////#endif
//ExceptionContext exceptionContext;

ExceptionContext& ExceptionContext::getCurrent()
{
  static thread_local ExceptionContext thisThreadsExceptionContext;
  return thisThreadsExceptionContext;
}


std::string valueList_to_string(const std::vector<double>& vals, size_t maxlen)
{
  std::ostringstream os;
  os <<"(";

  if (vals.size()>0)
  {
    size_t n1=std::min(vals.size(), maxlen-2);

    for (size_t i=0; i<n1; i++)
      os<<" "<<vals[i];

    if (n1<vals.size())
      {
        os << " .... "<<vals.back();
      }
  }
  os<<" )";
  return os.str();
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



void Warning(const std::string& msg)
{
  WarningDispatcher::getCurrent().issue( msg );
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
//    std::cerr << std::endl
//              << "An error has occurred:" << std::endl
//              << ie->message() << std::endl
//                 ;

    displayFramed(title.str(), ie->message(), '=', std::cerr);
    if (getenv("INSIGHT_STACKTRACE"))
    {
      std::cerr << "Stack trace:" << std::endl
                << ie->strace() <<std::endl;
    }
  }
  else
  {
//    std::cerr << std::endl
//              << "An error has occurred:" << std::endl
//              << e.what() << std::endl
//                 ;
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

UnsupportedFeature::UnsupportedFeature()
{}

UnsupportedFeature::UnsupportedFeature(const string &msg, bool strace)
  : Exception(msg, strace)
{}





}
