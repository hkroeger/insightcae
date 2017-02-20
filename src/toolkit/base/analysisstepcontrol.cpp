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

#include "analysisstepcontrol.h"

#include <iostream>
#include "base/analysis.h"

namespace insight
{
  
// void AnalysisStep::flushStepCache() const
// {
//   if (analysis_.writestepcache_)
//   {
//     std::ofstream f(analysis_.stepcachefile_.c_str(), std::ios_base::app);
//     f<<curstep_<<std::endl;
//   }
// }
// 
//   
// AnalysisStep::AnalysisStep(Analysis& analysis, const std::string& curstep)
// : analysis_(analysis),
//   curstep_(curstep)
// {
//   std::cout<<
//     " =>=>=> Entering next analysis step: "<<curstep_<<" <=<=<= "
//   <<std::endl;
// }
// 
// AnalysisStep::~AnalysisStep()
// {
//   std::cout<<
//     " =>=>=> analysis step finished: "<<curstep_<<" <=<=<= "
//   <<std::endl;
//   analysis_.performedsteps_.insert(curstep_);
//   flushStepCache();
// }
// 
// AnalysisStep::operator bool() const
// {
//   if ( analysis_.performedsteps_.find(curstep_) != analysis_.performedsteps_.end() )
//   {
//     std::cout<<
//       " =>=>=> (skip)"
//       <<std::endl;
//     return false;
//   }
//   else
//   {
//     return true;
//   }
// }

}
