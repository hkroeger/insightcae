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

#ifndef INSIGHT_CAD_ASTBASE_H
#define INSIGHT_CAD_ASTBASE_H

#include <set>
#include <thread>
#include <mutex>


namespace insight
{
namespace cad
{


class RebuildCancelException
{
};

/**
 * implements update-on-request framework
 */
class ASTBase
{
  bool valid_;
  mutable bool building_;

  
  static std::mutex cancel_mtx_;
  static std::set<std::thread::id> cancel_requests_;

protected:
  void setValid();

  mutable size_t hash_;
  virtual size_t calcHash() const =0;
  virtual void build() =0;

public:
  static void cancelRebuild(std::thread::id thread_id = std::this_thread::get_id());

  ASTBase();
  ASTBase(const ASTBase& o);
  virtual ~ASTBase();
  
  bool valid() const;
  bool building() const;

  virtual void checkForBuildDuringAccess() const;

  /**
   * @brief hash
   * @return
   * returns the hash, if it already computed. Triggers computation otherwise
   */
  size_t hash() const;

};


}
}

#endif // INSIGHT_CAD_ASTBASE_H
