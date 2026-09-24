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


#include "warningdispatcher.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <sstream>

#ifdef WIN32
#include <windows.h>  // FlsAlloc / FlsGetValue / FlsSetValue
#endif

using namespace std;

namespace insight
{


WarningDispatcher::WarningDispatcher()
  : superDispatcher_(nullptr)
{}

void WarningDispatcher::setSuperDispatcher(WarningDispatcher *superDispatcher)
{
  superDispatcher_=superDispatcher;
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

    std::lock_guard<std::mutex> lock(mutex_);
    warnings_.push_back(warning);
    for (const auto& cb : issueCallbacks_)
      cb.second(warning);
  }
}


int WarningDispatcher::addIssueCallback(
    std::function<void(const insight::Exception&)> cb)
{
  std::lock_guard<std::mutex> lock(mutex_);
  int id = nextCallbackId_++;
  issueCallbacks_[id] = std::move(cb);
  return id;
}

void WarningDispatcher::removeIssueCallback(int id)
{
  std::lock_guard<std::mutex> lock(mutex_);
  issueCallbacks_.erase(id);
}

void WarningDispatcher::clear()
{
  std::lock_guard<std::mutex> lock(mutex_);
  warnings_.clear();
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


WarningDispatcher& WarningDispatcher::getCurrent()
{
#ifdef WIN32
  // Same FLS-based fix as ExceptionContext::getCurrent() in exception.cpp.
  static DWORD flsIdx = FlsAlloc([](PVOID ptr) {
      delete static_cast<WarningDispatcher*>(ptr);
  });
  auto* p = static_cast<WarningDispatcher*>(FlsGetValue(flsIdx));
  if (!p) {
      p = new WarningDispatcher();
      FlsSetValue(flsIdx, p);
  }
  return *p;
#else
  static thread_local WarningDispatcher thisThreadsWarnings;
  return thisThreadsWarnings;
#endif
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


}
