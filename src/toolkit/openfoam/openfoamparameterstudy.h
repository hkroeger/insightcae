
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

#ifndef INSIGHT_OPENFOAMPARAMETERSTUDY_H
#define INSIGHT_OPENFOAMPARAMETERSTUDY_H

#include "openfoamanalysis.h"

namespace insight {

    
template<
 class BaseAnalysis,
 const RangeParameterList& var_params = RangeParameterList()
 >
class OpenFOAMParameterStudy
: public ParameterStudy<BaseAnalysis, var_params>
{
protected:
  bool subcasesRemesh_;
  boost::shared_ptr<OpenFOAMAnalysis> base_case_;
  
public:
//     declareType("OpenFOAM Parameter Study");
    
    OpenFOAMParameterStudy
    (
        const std::string& name, 
        const std::string& description, 
        const ParameterSet& ps,
        const boost::filesystem::path& exePath,
        bool subcasesRemesh=false
    );

    virtual void modifyInstanceParameters(const std::string& subcase_name, ParameterSetPtr& newp) const;
    virtual ResultSetPtr operator()(ProgressDisplayer* displayer = 0);

    virtual void evaluateCombinedResults(ResultSetPtr& results);
};


}

#include "openfoamparameterstudy.cpp"

#endif
