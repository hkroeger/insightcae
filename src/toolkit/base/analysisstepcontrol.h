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

#include <string>
#include <vector>
#include <boost/preprocessor.hpp>


namespace insight
{
  
// #define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE(r, data, elem)    \
//     case elem : return std::string(BOOST_PP_STRINGIZE(elem));
// 
// #define DEFINE_ANALYSIS_STEPS(enumerators)                \
//     enum AnalysisSteps {                                                               \
//         BOOST_PP_SEQ_ENUM(enumerators)                                        \
//     };                                                                        \
//                                                                               \
//     inline std::string stepIDToString(AnalysisSteps v)                                       \
//     {                                                                         \
//         switch (v)                                                            \
//         {                                                                     \
//             BOOST_PP_SEQ_FOR_EACH(                                            \
//                 X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,          \
//                 AnalysisSteps,                                                         \
//                 enumerators                                                   \
//             )                                                                 \
//             default: return "[Unknown " BOOST_PP_STRINGIZE(AnalysisSteps) "]";         \
//         }                                                                     \
//     }
/*
#define X_DEFINE_ANALYSIS_STEP_CASE(r, data, elem)    \
    const std::string elem = BOOST_PP_STRINGIZE(elem);

#define DEFINE_ANALYSIS_STEPS(enumerators)            \
            BOOST_PP_SEQ_FOR_EACH(                    \
                X_DEFINE_ANALYSIS_STEP_CASE,          \
                AnalysisSteps,                        \
                enumerators                           \
            )
 
class AnalysisTracker
{
protected:
  std::vector<std::string> completedSteps_;
  
public:
  
  class Step
  {
  public:
    Step(AnalysisTracker<A>& tracker);
    
    bool isFinished() const;
    inline operator bool() const { return isFinished(); }
    
    void markCompleted();
  };
  
  AnalysisTracker(const StepList& steps);
};
*/
}
