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
 */

#include "boost/thread.hpp"
#include "astbase.h"


namespace insight
{
namespace cad
{

std::mutex ASTBase::cancel_mtx_;
std::set<std::thread::id> ASTBase::cancel_requests_;

void ASTBase::cancelRebuild(std::thread::id thread_id)
{
  std::lock_guard<std::mutex> l(cancel_mtx_);
  cancel_requests_.insert(thread_id);
}

  
ASTBase::ASTBase()
: valid_(false),
  building_(false)
{}


ASTBase::~ASTBase()
{}


void ASTBase::checkForBuildDuringAccess() const
{
  {
    std::lock_guard<std::mutex> l(cancel_mtx_);
    auto i = cancel_requests_.find(std::this_thread::get_id());
    if ( i != cancel_requests_.end())
      {
        cancel_requests_.erase(i);
        throw RebuildCancelException();
      }
  }

  boost::mutex m_mutex;
  boost::unique_lock<boost::mutex> lock(m_mutex);
  
  if (!valid()) 
  {
    building_=true;
    const_cast<ASTBase*>(this)->build();
    building_=false;
    const_cast<ASTBase*>(this)->setValid();
  }
}

}
}
