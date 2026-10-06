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


#ifndef INSIGHT_WARNINGDISPATCHER_H
#define INSIGHT_WARNINGDISPATCHER_H

#include "toolkit_export.h"

#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "base/exception.h"


namespace insight {


class ScopedWarningRecorder;


class WarningDispatcher
{
  friend class ScopedWarningRecorder;

  WarningDispatcher *superDispatcher_=nullptr;
  std::vector<insight::Exception> warnings_;

  /**
   * active recorders of the thread, which owns this dispatcher.
   * Only accessed from that thread.
   */
  std::vector<ScopedWarningRecorder*> recorders_;

  /**
   * true while a warning, which was captured by a recorder,
   * is forwarded and its callbacks are executed (in the issuing thread)
   */
  bool currentIssueRecorded_=false;

  mutable std::mutex mutex_;
  std::map<int, std::function<void(const insight::Exception&)>> issueCallbacks_;
  int nextCallbackId_=0;

public:
  WarningDispatcher();
  void setSuperDispatcher(WarningDispatcher* superDispatcher);

  void issue(const std::string& message);
  void issue(const insight::Exception& warning);

  /**
   * Register a callback that is invoked whenever a warning is issued on this dispatcher.
   * May be called from any thread. Returns an ID that can be passed to removeIssueCallback().
   */
  int addIssueCallback(std::function<void(const insight::Exception&)> cb);
  void removeIssueCallback(int id);

  /** Clear the accumulated warning list (does not affect callbacks). */
  void clear();

  const decltype(warnings_)& warnings() const;
  size_t nWarnings() const;

  static WarningDispatcher& getCurrent();

  /**
   * @brief currentIssueIsRecorded
   * To be called from issue callbacks.
   * @return
   * true, if the warning, which is currently dispatched,
   * has been captured by a ScopedWarningRecorder in the issuing thread.
   */
  static bool currentIssueIsRecorded();

};




/**
 * @brief The ScopedWarningRecorder class
 * captures all warnings, which are issued in the current thread
 * while this object exists. The warnings are dispatched as usual in addition.
 * If recorders are nested, only the innermost one receives the warnings.
 */
class ScopedWarningRecorder
{
  friend class WarningDispatcher;

public:
  typedef std::function<void(const insight::Exception&)> Sink;

private:
  Sink sink_;
  WarningDispatcher& dispatcher_;

public:
  explicit ScopedWarningRecorder(Sink sink);
  ~ScopedWarningRecorder();

  ScopedWarningRecorder(const ScopedWarningRecorder&) = delete;
  ScopedWarningRecorder& operator=(const ScopedWarningRecorder&) = delete;
};


void displayFramed(const std::string& title, const std::string& msg, char titleChar = '=', std::ostream &os = std::cerr);


void Warning(std::string msgfmt, ...);
void Warning(const std::exception& ex);


}

#endif // INSIGHT_WARNINGDISPATCHER_H
