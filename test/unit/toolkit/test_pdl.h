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

#ifndef INSIGHT_TEST_PDL_H
#define INSIGHT_TEST_PDL_H

#include "base/boost_include.h"
#include "base/parameterset.h"
#include "base/parameters.h"
#include "test_pdl__TestPDL__Parameters_headers.h"

namespace insight {


class TestPDL
{

public:
#include "test_pdl__TestPDL__Parameters.h"
/*
PARAMETERSET>>> TestPDL Parameters

L = dimensionedScalar Length millimeters 1.0 "One millimeter"

run = set {

 regime = selectablesubset
 {{

    steady
    set {
        iter = int 30000 "number of outer iterations after which the solver should stop"
    }

    unsteady
    set {
        endTime = double 10.0 "dimensionless simulation time (L/vs) at which the solver should stop"
    }

 }} unsteady "The simulation regime" *hidden


 startup = selectablesubset {{

   fullSpeed
   set { }

   rampSpeed
   set {
     v0 = double 0.0 "[m/s] initial speed at t=0"
     T = double 1.0 "[-] Non-dimensional ramp up time"
   }

  }} fullSpeed "How to start the simulation: ramp up the speed from rest or begin with full speed."


 initialization = set {

  coarsestLevelInit = selectablesubset {{

   none
   set { }

   ramp
   set {
     v0 = double 0.0 "[m/s] initial speed"
     T = double 1.0 "[-] Non-dimensional ramp up time"
   }

  }} ramp "Initialization option for the coarsest pre-run. If no coarse pre-runs are selected, these options apply to the final simulation."

  preRuns = set {

    resolutions = array [ set {
      nax_parameter = double 0.33 "Resolution parameter of the coarse setup (nax value or fraction of final nax)."
      np_coarse = int -1 "Number of processors to use for coarse run. Set to -1 to use the same number as for the final run."
    } ]*1 ""

    resolution_parameter = selection (fixed scaled) scaled "Meaning of the parameter nax_parameter in the specified resolutions."
  }

 }

} "Solver parameters"

<<<PARAMETERSET
*/


};

}

#endif
