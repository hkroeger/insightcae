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

#ifndef INSIGHT_HOMOGENEOUSAVERAGEDPROFILE_H
#define INSIGHT_HOMOGENEOUSAVERAGEDPROFILE_H

#include "base/analysis.h"
#include "base/resultset.h"
#include "base/parameters/simpleparameter.h"
#include "base/supplementedinputdata.h"

namespace insight
{

class HomogeneousAveragedProfile
: public AnalysisWithParameters
{
public:
#include "homogeneousaveragedprofile__HomogeneousAveragedProfile__Parameters.h"
/*
PARAMETERSET>>> HomogeneousAveragedProfile Parameters
inherits AnalysisWithParameters::Parameters

OFEname = string "OF23x" "Name of OpenFOAM installation"
casepath = path "." "Path to OpenFOAM case"
profile_name= string "radial" "name of the profile (controls name of generated sample sets, please avoid interference with other profile names)"

p0 = vector (0 0 0) "start point of profile"
L = vector (0 1 0) "length and direction of profile"
np = int 100 "number of sampling points"
grading = selection (
  none towardsStart towardsEnd towardsBoth
) none "Definition of grading direction"
homdir1 = vector (3.14 0 0) "direction and span of homogeneous averaging direction 1"
n_homavg1 = int 10 "number of homogeneous averaging samples in direction 1 (value 1 switches direction off)"
homdir2 = vector (0 0 1) "direction and span of homogeneous averaging direction 2"
n_homavg2 = int 1 "number of homogeneous averaging samples in direction 2  (value 1 switches direction off)"

fields = array [
 string "UMean" "Field name"
 ]*1 "Names of fields for sampling"

<<<PARAMETERSET
*/

    typedef supplementedInputDataDerived<Parameters> supplementedInputData;
    addParameterMembers_SupplementedInputData(HomogeneousAveragedProfile::Parameters);

public:
  declareType("HomogeneousAveragedProfile");

  HomogeneousAveragedProfile(
      const std::shared_ptr<supplementedInputDataBase>& sp );

  ResultSetPtr operator()(ProgressDisplayer& displayer = consoleProgressDisplayer) override;

  static std::string category() { return "General Postprocessing"; }
  static AnalysisDescription description() { return {"Homogeneous Averaged Profile", ""}; }
};

}

#endif // INSIGHT_HOMOGENEOUSAVERAGEDPROFILE_H
