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


#ifndef INSIGHT_EXCEPTION_H
#define INSIGHT_EXCEPTION_H

#include "toolkit_export.h"

#include <exception>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <armadillo>
#include <memory>

namespace insight {
  


class CurrentExceptionContext
        : public std::string
{
    int verbosityLevel_;

    void start(const char* msg);

public:
  CurrentExceptionContext(int verbosityLevel, std::string msgfmt, ...);
  CurrentExceptionContext(std::string msgfmt, ...);
  ~CurrentExceptionContext();

  std::string contextDescription() const;

};


std::ostream& dbg(int verbosityLevel = 1);




std::string splitMessage
(
    const std::string& message,
    std::size_t width = 80,
    std::string begMark=std::string(),
    std::string endMark=std::string(),
    std::string whitespace = " \t\r"
);




class Exception;
  



std::ostream& operator<<(std::ostream& os, const Exception& ex);




namespace cad
{
class Feature;
typedef std::shared_ptr<Feature> FeaturePtr;
}




class Exception
: public std::exception
{

  std::string message_;
  std::string strace_;

  std::map<std::string, cad::FeaturePtr> contextGeometry_;

  void saveContext(bool strace);

  mutable std::string whatMessage_;
  
public:
  Exception();
  Exception(std::string msgfmt, ...);
  Exception(const std::string& msg, const std::map<std::string, cad::FeaturePtr>& contextGeometry, bool strace=true);
  // Exception(const std::string& msg, const std::string& strace);


  virtual std::string message() const;
  inline std::string& messageRef() { return message_; }

  inline std::string as_string() const { return static_cast<std::string>(*this); }
  operator std::string() const;
  inline const std::string& strace() const { return strace_; }

  const char* what() const noexcept override;
  const std::map<std::string, cad::FeaturePtr>& contextGeometry() const;

  friend std::ostream& operator<<(std::ostream& os, const Exception& ex);
};


class ExternalProcessFailed
    : public Exception
{
  int retcode_;
  std::string exename_, errout_;

public:
  ExternalProcessFailed();
  ExternalProcessFailed(int retcode, const std::string& exename, const std::string& errout);

  std::string message() const override;
  const std::string& exeName() const;
};


class UnsupportedFeature
    : public Exception
{
public:
  UnsupportedFeature();
  UnsupportedFeature(const std::string& msg, bool strace=true);
};




void assertion(bool condition, std::string context_message, ...);


//std::string valueList_to_string(const std::vector<double>& vals, size_t maxlen=5);

template<class Container>
std::string valueList_to_string(const Container& vals, size_t maxlen)
{
  std::ostringstream os;
  os <<"(";

  if (vals.size()>0)
  {
    size_t n1=std::min(vals.size(), maxlen-2);

    auto ii=vals.begin();
    for (size_t i=0; i<n1; ++i)
    {
      os<<" "<<(*ii);
      ++ii;
    }

    if (n1<vals.size())
      {
        os << " .... "<<(*(--vals.end()));
      }
  }
  os<<" )";
  return os.str();
}

std::string valueList_to_string(const arma::mat& vals, arma::uword maxlen=5);
std::string vector_to_string(const arma::mat& vals, bool addMag=true);




class ExceptionContext
: public std::vector<CurrentExceptionContext*>
{
public:
  void snapshot(std::vector<std::string>& context);

  static ExceptionContext& getCurrent();
};




//extern
//#ifndef WIN32
//thread_local
//#endif
//ExceptionContext exceptionContext;




class WarningDispatcher
{

  WarningDispatcher *superDispatcher_=nullptr;
  std::vector<insight::Exception> warnings_;


public:
  WarningDispatcher();
  void setSuperDispatcher(WarningDispatcher* superDispatcher);

  void issue(const std::string& message);
  void issue(const insight::Exception& warning);


  const decltype(warnings_)& warnings() const;
  size_t nWarnings() const;

  static WarningDispatcher& getCurrent();

};

//extern
//#ifndef WIN32
//thread_local
//#endif
//WarningDispatcher warnings;

void displayFramed(const std::string& title, const std::string& msg, char titleChar = '=', std::ostream &os = std::cerr);


void Warning(std::string msgfmt, ...);
void Warning(const std::exception& ex);


class UnhandledExceptionHandling
{
public:
  static void handler();

  UnhandledExceptionHandling();
};



void printException(const std::exception& e);


}

#endif // INSIGHT_EXCEPTION_H
