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
#include "openfoam/caseelements/turbulencemodel.h"

namespace insight {


const boost::filesystem::path libSubDir = "brake";

class BrakePad
{
public:
    declareType("brakePad");

    BrakePad(rapidxml::xml_node<>& padNode)
    {}
};

defineType(BrakePad);

typedef PropertyLibrary<BrakePad, &libSubDir> BrakePads;


class TestPDL
{

public:
#include "test_pdl__TestPDL__Parameters.h"
/*
PARAMETERSET>>> TestPDL Parameters

L = dimensionedScalar Length millimeters 1.0 "One millimeter"

ap = array [ double 1. "" ] *2 ""

dr = doubleRange ( 1. 2. 3. ) ""

matrix = matrix [ [ 1., 2. ],  [ 3., 4. ] ] ""

sel = selection ( one two three ) one ""

trsf = spatialTransformation (0 0 0) ( 0 0 0) 1 ""

mapFrom 	= 	path 	"" 	"Map solution from specified case, if not empty. potentialinit is skipped if specified."

turbulenceModel = dynamicclassparameters "insight::turbulenceModel" default "kOmegaSST" "Turbulence model"

sketch = cadsketch
        ""
        ""
        ""
        "contour to extrude"

geometry = set {
      walls = labeledarray "wall_%d" [ set {
        file = path "" 	"Part of the geometry, excluding in- and outlet. May be an STL file or CAD exchange format (STEP or IGES)." *necessary
      } ] *1 "Pieces of geometry. All pieces together must completely resemble the perimeter of the internal channel."
      inlet = path "" "Triangulated geometry of inlet alone. May be an STL file or CAD exchange format (STEP or IGES)." *necessary
      outlet = path "" "Triangulated geometry of outlet alone. May be an STL file or CAD exchange format (STEP or IGES)." *necessary

    model = librarySelection "insight::BrakePads" RH250 "Select the model" *necessary

} "Specification of geometry"


wallBCs = labeledarray keysFrom "geometry/walls" [ selectablesubset {{
     adiabatic set {}
     fixedTemperature set {
      wallTemperature = double 300 "[K] Fixed temperature of the walls"
     }
}} adiabatic "" ] *0 ""

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
    } ]*2 ""

    resolution_parameter = selection (fixed scaled) scaled "Meaning of the parameter nax_parameter in the specified resolutions."
  }

 }

} "Solver parameters"

operation = set {

  DICOMdata = directory "" "Directory with raw data from scan." *necessary

}
<<<PARAMETERSET
*/


};

}

#endif
