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

#ifndef CONVERGENCEANALYSIS_H
#define CONVERGENCEANALYSIS_H

#include <base/analysis.h>

namespace insight
{
  
  
class ConvergenceAnalysis
: public Analysis
{
public:
#include "convergenceanalysis__ConvergenceAnalysis__Parameters.h"
/*
PARAMETERSET>>> ConvergenceAnalysis Parameters

xname = string "deltax" "Refinement parameter name (for proper report generation)"
name = string "S" "Solution quantity name (for proper report generation)"
yunit = string "" "Unit of solution quantity (for proper report generation)"

p_est = double 2 "Expected convergence order"

solutions = array [ set {
  deltax = double 1 "Value of refined quantity (e.g. mesh size $\\Delta x$). Smaller value means finer resolution."
  S = double 1 "Solution value"
 } ] *3 "Solution quantity on different resolution levels. The list has to contain 3 elements."

<<<PARAMETERSET
*/

protected:
    Parameters p_;
  
public:
    declareType("ConvergenceAnalysis");
    
    ConvergenceAnalysis(const ParameterSet& ps, const boost::filesystem::path& exepath);
    
    virtual ResultSetPtr operator()(ProgressDisplayer* displayer=NULL);
    
    static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
    static std::string category() { return "General Postprocessing"; }
};

}

#endif
