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
  if (strace)
  {
    int num=10;
    void *array[num];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, num);

    // print out all the frames to stderr
    char **str=backtrace_symbols(array, size);

    ostringstream oss;
    for (size_t i=0; i<size; i++)
    {
      oss<<str[i]<<endl;
      //free(str[i]);
    }
    //free(str);

    strace_=oss.str();
  }
  else
    strace_="";
}

Exception::~Exception()
{
}

Exception::operator std::string() const
{
  if (strace_!="")
    return "\n\nERROR MESSAGE:\n\n"+message_+"\n\n\nSTACK TRACE:\n"+strace_+"\n\n";
  else
    return message_;
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