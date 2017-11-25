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

#ifndef INSIGHT_ANALYSISSTEPCONTROL_H
#define INSIGHT_ANALYSISSTEPCONTROL_H

#include <string>
#include <vector>
#include <set>
#include <boost/preprocessor.hpp>


namespace insight
{
  
// class Analysis;
//  
// typedef std::set<std::string> AnalysisStepList;
// 
// #define INSIGHT_ANALYSIS_STEP_NOT_DONE(analysis, curstep) AnalysisStep cs=AnalysisStep(analysis, curstep)
//  
// class AnalysisStep
// {
//   Analysis& analysis_;
//   std::string curstep_;
//   
//   void flushStepCache() const;
//   
// public:
//   AnalysisStep(Analysis& analysis, const std::string& curstep);
//   ~AnalysisStep();
//   
//   /**
//    * return true, if the step is ok to execute, i.e. has not been performed
//    */
//   operator bool() const;
// };

}

#endif
