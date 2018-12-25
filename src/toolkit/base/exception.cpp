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
#include <execinfo.h>
#include <sstream>
#include <cstdlib>


#include <execinfo.h> // for backtrace
#include <dlfcn.h>    // for dladdr
#include <cxxabi.h>   // for __cxa_demangle
#include <cstdio>
#include <cstdlib>

using namespace std;

namespace insight
{
  
std::ostream& operator<<(std::ostream& os, const Exception& ex)
{
  os<<static_cast<std::string>(ex);
  return os;
}


Exception::Exception(const std::string& msg, bool strace)
{
  message_=msg;

  exceptionContext.snapshot(context_);

  if (strace)
  {
    int num_max=30;
    void *callstack[num_max];

    // get void*'s for all entries on the stack
    int nFrames = backtrace(callstack, num_max);

    // print out all the frames to stderr
    char **symbols=backtrace_symbols(callstack, nFrames);

    ostringstream trace_buf;

    char buf[1024];
    for (int i=0; i<nFrames; i++)
    {
        Dl_info info;
        if (dladdr(callstack[i], &info) && info.dli_sname) {
            char *demangled = NULL;
            int status = -1;
            if (info.dli_sname[0] == '_')
                demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
            snprintf
                (
                  buf, sizeof(buf),
//                  "%-3d %*p %s + %zd\n",
                  "%-3d %s\n",
                      i,
//                      int(2 + sizeof(void*) * 2),
//                      callstack[i],
                      (
                        (status == 0) ?
                        demangled :
                        info.dli_sname == 0 ? symbols[i] : info.dli_sname
                      )/*,
                      (char *)callstack[i] - (char *)info.dli_saddr*/
                     );
            free(demangled);
        } else {
            snprintf
                (
                  buf, sizeof(buf),
//                    "%-3d %*p %s\n",
                    "%-3d %s\n",
                     i,
//                     int(2 + sizeof(void*) * 2),
//                     callstack[i],
                     symbols[i]
                );
        }
        trace_buf << buf;
//      trace_buf<<symbols[i]<<endl;
      //free(str[i]);
    }
    //free(str);

    strace_=trace_buf.str();
  }
  else
    strace_="";
}

Exception::~Exception()
{
}

Exception::operator std::string() const
{
  string context="";
  for (const auto& c: context_)
    {
      context+= "\nwhile "+string(c);
    }
  if (strace_!="")
    return
        "\n\n"
        "ERROR MESSAGE:"
        "\n\n"
        +message_
        +context+
        "\n\n\n"
        "STACK TRACE:\n"
        +strace_+
        "\n\n";
  else
    return message_+context;
}


CurrentExceptionContext::CurrentExceptionContext(const std::string& desc)
: desc_(desc)
{
  exceptionContext.push_back(this);
}


CurrentExceptionContext::~CurrentExceptionContext()
{
  if (exceptionContext.back()==this)
    exceptionContext.pop_back();
  else
    {
      std::cerr<<"Oops: CurrentExceptionContext destructor: expected to be last!"<<endl;
    }
}

void ExceptionContext::snapshot(std::vector<std::string>& context)
{
  context.clear();
  for (const auto& i: *this)
    {
      context.push_back(*i);
    }
}

thread_local ExceptionContext exceptionContext;

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


void Warning(const std::string& msg)
{
  std::cout<<"\n\n"
"================================================================\n"
"================================================================\n"
"    WW      WW   AAA   RRRRRR  NN   NN IIIII NN   NN   GGGG     \n"
"    WW      WW  AAAAA  RR   RR NNN  NN  III  NNN  NN  GG  GG    \n"
"    WW   W  WW AA   AA RRRRRR  NN N NN  III  NN N NN GG         \n"
"     WW WWW WW AAAAAAA RR  RR  NN  NNN  III  NN  NNN GG   GG    \n"
"      WW   WW  AA   AA RR   RR NN   NN IIIII NN   NN  GGGGGG    \n"
"================================================================\n"
    <<std::endl;
    
  std::cout<<msg<<std::endl;
  
  std::cout<<std::endl<<
"================================================================\n"
"================================================================\n\n"
    <<std::endl;
}

void UnhandledExceptionHandling::handler()
{
    void *trace_elems[20];
    int trace_elem_count(backtrace( trace_elems, 20 ));
    char **stack_syms(backtrace_symbols( trace_elems, trace_elem_count ));
    for ( int i = 0 ; i < trace_elem_count ; ++i )
    {
        std::cout << stack_syms[i] << "\n";
    }
    free( stack_syms );

    exit(1);
}

UnhandledExceptionHandling::UnhandledExceptionHandling()
{
    std::set_terminate( handler );
}

}
