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
#include "base/propertylibrary.h"
#include "base/parameterset.h"
#include "base/parameters.h"
#include "base/parameters/spatialtransformationparameter.h"
#include "base/parameters/simpledimensionedparameter.h"
#include "openfoam/caseelements/turbulencemodel.h"

#include "test_pdl__SubPS__Parameters_headers.h"
#include "test_pdl__TestPDL__Parameters_headers.h"

namespace insight {


const boost::filesystem::path libSubDir = "tests";

class BrakePad
{
public:
    declareType("test");

    BrakePad(rapidxml::xml_node<>& padNode)
    {}
};

defineType(BrakePad);

typedef PropertyLibrary<BrakePad, &libSubDir> BrakePads;



class SubPS
{
public:
#include "test_pdl__SubPS__Parameters.h"
/*
PARAMETERSET>>> SubPS Parameters

W = dimensionedScalar Length millimeters 1.0 "One millimeter"

subInt    = int    1       "int in includedset"
subBool   = bool   false   "bool in includedset"
subString = string "sub"   "string in includedset"
subDate   = date   2024-01-15       "date in includedset"
subDt     = datetime 2024-01-15 09:00  "datetime in includedset"
subVec    = vector (1.0 0.0 0.0)    "vector in includedset"
subMat    = matrix [[1.0, 0.0], [0.0, 1.0]] "matrix in includedset"
subSel    = selection (subSelA subSelB subSelC) subSelA "selection in includedset"
subDr     = doubleRange (1.0 2.0 3.0) "doubleRange in includedset"
subPath   = path ""  "path in includedset"
subDir    = directory "" "directory in includedset"
subTrsf   = spatialTransformation (0 0 0) (0 0 0) 1 "spatialTransformation in includedset"

subss = selectablesubset {{

    subssChoiceX set {
        subssxDbl  = double 1.0         "double in selectablesubset in includedset"
        subssxInt  = int    1           "int in selectablesubset in includedset"
        subssxBool = bool   true        "bool in selectablesubset in includedset"
        subssxStr  = string "x"         "string in selectablesubset in includedset"
        subssxDate = date   2024-03-01  "date in selectablesubset in includedset"
        subssxVec  = vector (1.0 0.0 0.0) "vector in selectablesubset in includedset"
        subssxMat  = matrix [[1.0, 0.0], [0.0, 1.0]] "matrix in selectablesubset in includedset"
        subssxSel  = selection (subssxP subssxQ) subssxP "selection in selectablesubset in includedset"
        subssxDr   = doubleRange (1.0 2.0) "doubleRange in selectablesubset in includedset"
        subssxPath = path ""    "path in selectablesubset in includedset"
        subssxDir  = directory "" "directory in selectablesubset in includedset"
        subssxTrsf = spatialTransformation (1 0 0) (0 0 0) 1 "trsf in selectablesubset in includedset"
    }

    subssChoiceY set {
        subssyDbl = double 2.0 "double in alternative B in includedset"
    }

}} subssChoiceX "selectablesubset in includedset"

subArr = array [ set {
    subArrDbl  = double 1.0         "double in array-element-set in includedset"
    subArrInt  = int    1           "int in array-element-set in includedset"
    subArrBool = bool   false       "bool in array-element-set in includedset"
    subArrStr  = string "a"         "string in array-element-set in includedset"
    subArrDate = date   2024-04-01  "date in array-element-set in includedset"
    subArrVec  = vector (0.0 1.0 0.0) "vector in array-element-set in includedset"
    subArrMat  = matrix [[2.0, 0.0], [0.0, 2.0]] "matrix in array-element-set in includedset"
    subArrSel  = selection (subArrR subArrS subArrT) subArrR "selection in array-element-set in includedset"
    subArrDr   = doubleRange (4.0 5.0 6.0) "doubleRange in array-element-set in includedset"
    subArrPath = path ""   "path in array-element-set in includedset"
    subArrDir  = directory "" "directory in array-element-set in includedset"
    subArrTrsf = spatialTransformation (0 1 0) (0 0 0) 1 "trsf in array-element-set in includedset"
} ] *2 "array with comprehensive set elements in includedset"

subLabArr = labeledarray "subla_%d" [ set {
    subLaDbl  = double 1.0         "double in labeledarray-element-set in includedset"
    subLaInt  = int    1           "int in labeledarray-element-set in includedset"
    subLaBool = bool   false       "bool in labeledarray-element-set in includedset"
    subLaStr  = string "la"        "string in labeledarray-element-set in includedset"
    subLaDate = date   2024-05-01  "date in labeledarray-element-set in includedset"
    subLaVec  = vector (0.0 0.0 1.0) "vector in labeledarray-element-set in includedset"
    subLaMat  = matrix [[3.0, 0.0], [0.0, 3.0]] "matrix in labeledarray-element-set in includedset"
    subLaSel  = selection (subLaM subLaN subLaO) subLaM "selection in labeledarray-element-set in includedset"
    subLaDr   = doubleRange (7.0 8.0 9.0) "doubleRange in labeledarray-element-set in includedset"
    subLaPath = path ""   "path in labeledarray-element-set in includedset"
    subLaDir  = directory "" "directory in labeledarray-element-set in includedset"
    subLaTrsf = spatialTransformation (0 0 1) (0 0 0) 1 "trsf in labeledarray-element-set in includedset"
} ] *1 "labeledarray with comprehensive set elements in includedset"

subsub = set {
 sarr        = array [ double 1. "" ] *2 ""
 subsubDbl   = double  2.0         "double in set in includedset"
 subsubInt   = int     2           "int in set in includedset"
 subsubBool  = bool    true        "bool in set in includedset"
 subsubStr   = string  "ss"        "string in set in includedset"
 subsubDate  = date    2024-02-15  "date in set in includedset"
 subsubDt    = datetime 2024-02-15 07:00 "datetime in set in includedset"
 subsubVec   = vector  (0.0 1.0 0.0) "vector in set in includedset"
 subsubMat   = matrix  [[2.0, 0.0], [0.0, 2.0]] "matrix in set in includedset"
 subsubSel   = selection (subsubAA subsubBB subsubCC) subsubAA "selection in set in includedset"
 subsubDr    = doubleRange (4.0 5.0 6.0) "doubleRange in set in includedset"
 subsubPath  = path ""   "path in set in includedset"
 subsubDir   = directory "" "directory in set in includedset"
 subsubTrsf  = spatialTransformation (0 0 0) (0 0 0) 1 "trsf in set in includedset"
} ""

<<<PARAMETERSET
*/

};



class Outline_SketchParameters
    : public insight::cad::ConstrainedSketchParametersDelegate
{
public:
public:
#include "test_pdl__Outline_SketchParameters__Parameters.h"
/*
PARAMETERSET>>> Outline_SketchParameters Parameters

L = double 1.0 "[mm] One millimeter"

outlinesub = set {
 karr = array [ double 1. "" ] *2 ""
} ""

<<<PARAMETERSET
*/


    void
    changeDefaultParameters(insight::cad::ConstrainedSketchEntity& e) const override
    {
        if (dynamic_cast<const insight::cad::SketchPoint*>(&e))
        {
            e.changeDefaultParameters(
                *defaultParameters() );
        }
    }

    std::unique_ptr<insight::cad::LayerProperties>
    createDefaultLayerProperties(const std::string& layerName) const override
    {
        auto defp=defaultParameters();

        arma::mat c; // unspecified color
        if (layerName!=insight::cad::ConstrainedSketch::defaultLayerName)
            c=insight::vec3(std::rand(), std::rand(), std::rand())
                /double(RAND_MAX);

        return insight::cad::LayerProperties::create(
            *defp, c);
    }
};

static std::shared_ptr<insight::cad::ConstrainedSketchParametersDelegate>
    outline_SketchParameters
    = std::make_shared<Outline_SketchParameters>();

class TestPDL
{

public:
#include "test_pdl__TestPDL__Parameters.h"
/*
PARAMETERSET>>> TestPDL Parameters

subps = includedset "insight::SubPS::Parameters" ""

L = dimensionedScalar Length millimeters 1.0 "One millimeter"

ap = array [ double 1. "" ] *2 ""

dr = doubleRange ( 1. 2. 3. ) ""

matrix = matrix [ [ 1., 2. ],  [ 3., 4. ] ] ""

sel = selection ( one two three ) one ""

trsf = spatialTransformation (0 0 0) ( 0 0 0) 1 ""

mapFrom 	= 	path 	"" 	"Map solution from specified case, if not empty. potentialinit is skipped if specified."

relFile = path "hull.stl" "A relative path to some file"

absFile = path "/hull.stl" "An absolute path to some file"

turbulenceModel = dynamicclassparameters "insight::turbulenceModel" default "kOmegaSST" "Turbulence model"

sketch = cadsketch
        ""
        "outline_SketchParameters"
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

myBool      = bool   true              "A boolean parameter"
myString    = string "hello world"     "A string parameter"
myDate      = date   2024-01-15        "A date parameter"
myDateTime  = datetime 2024-01-15 10:30 "A datetime parameter"
myVec       = vector (1.0 2.0 3.0)    "A non-spatial vector parameter"
myDirection = vector direction (0.0 0.0 1.0) "A direction vector parameter"
myPoint     = vector point (1.0 0.0 0.0)     "A point parameter"

allTypes = set {

    atsDbl   = double  1.0          "double inside set"
    atsInt   = int     5            "int inside set"
    atsBool  = bool    true         "bool inside set"
    atsStr   = string  "text"       "string inside set"
    atsDate  = date    2024-06-01   "date inside set"
    atsDt    = datetime 2024-06-01 12:00 "datetime inside set"
    atsVec   = vector  (0.0 1.0 0.0) "vector inside set"
    atsMat   = matrix  [[1.0, 0.0], [0.0, 1.0]] "matrix inside set"
    atsDimSc = dimensionedScalar Length meters 2.0 "dimensioned scalar inside set"
    atsSel   = selection (atsCat atsDog atsBird) atsCat "selection inside set"
    atsDr    = doubleRange (10.0 20.0 30.0) "doubleRange inside set"
    atsPath  = path   ""  "path inside set"
    atsDir   = directory "" "directory inside set"
    atsTrsf  = spatialTransformation (0 0 0) (0 0 0) 1 "spatialTransformation inside set"

    atsSS = selectablesubset {{

        atssChoiceA set {
            atssaDbl  = double  1.0         "double in selectablesubset in set"
            atssaInt  = int     1           "int in selectablesubset in set"
            atssaBool = bool    true        "bool in selectablesubset in set"
            atssaStr  = string  "hello"     "string in selectablesubset in set"
            atssaDate = date    2024-03-01  "date in selectablesubset in set"
            atssaDt   = datetime 2024-03-01 08:00 "datetime in selectablesubset in set"
            atssaVec  = vector  (1.0 0.0 0.0) "vector in selectablesubset in set"
            atssaMat  = matrix  [[1.0, 0.0], [0.0, 1.0]] "matrix in selectablesubset in set"
            atssaSel  = selection (atssaUp atssaDown) atssaUp "selection in selectablesubset in set"
            atssaDr   = doubleRange (1.0 2.0) "doubleRange in selectablesubset in set"
            atssaPath = path ""  "path in selectablesubset in set"
            atssaDir  = directory "" "directory in selectablesubset in set"
            atssaTrsf = spatialTransformation (1 0 0) (0 0 0) 1 "trsf in selectablesubset in set"
            atssaArr  = array [ double 1.0 "" ] *2 "array in selectablesubset in set"
            atssaLabArr = labeledarray "atssala_%d" [ set {
                atssalaA = double 1.0 ""
                atssalaB = int    1   ""
            } ] *1 "labeledarray in selectablesubset in set"
        }

        atssChoiceB set {
            atssbDbl = double 2.0 "double in alternative B in set"
        }

    }} atssChoiceA "selectablesubset inside set"

    atsArr = array [ set {
        atsarrDbl  = double  1.0        "double in array-element-set in set"
        atsarrInt  = int     1          "int in array-element-set in set"
        atsarrBool = bool    false      "bool in array-element-set in set"
        atsarrStr  = string  "elem"     "string in array-element-set in set"
        atsarrDate = date    2024-07-01 "date in array-element-set in set"
        atsarrDt   = datetime 2024-07-01 06:00 "datetime in array-element-set in set"
        atsarrVec  = vector  (0.0 0.0 1.0) "vector in array-element-set in set"
        atsarrMat  = matrix  [[2.0, 0.0], [0.0, 2.0]] "matrix in array-element-set in set"
        atsarrSel  = selection (atsarrP atsarrQ atsarrR) atsarrP "selection in array-element-set in set"
        atsarrDr   = doubleRange (5.0 10.0) "doubleRange in array-element-set in set"
        atsarrPath = path ""  "path in array-element-set in set"
        atsarrDir  = directory "" "directory in array-element-set in set"
        atsarrTrsf = spatialTransformation (0 1 0) (0 0 0) 1 "trsf in array-element-set in set"
    } ] *2 "array with comprehensive set elements in set"

    atsLabArr = labeledarray "atsla_%d" [ set {
        atslaDbl  = double  1.0        "double in labeledarray-element-set in set"
        atslaInt  = int     1          "int in labeledarray-element-set in set"
        atslaBool = bool    false      "bool in labeledarray-element-set in set"
        atslaStr  = string  "la"       "string in labeledarray-element-set in set"
        atslaDate = date    2024-08-01 "date in labeledarray-element-set in set"
        atslaDt   = datetime 2024-08-01 07:00 "datetime in labeledarray-element-set in set"
        atslaVec  = vector  (1.0 1.0 0.0) "vector in labeledarray-element-set in set"
        atslaMat  = matrix  [[3.0, 0.0], [0.0, 3.0]] "matrix in labeledarray-element-set in set"
        atslaSel  = selection (atslaU atslaV atslaW) atslaU "selection in labeledarray-element-set in set"
        atslaDr   = doubleRange (100.0 200.0) "doubleRange in labeledarray-element-set in set"
        atslaPath = path ""  "path in labeledarray-element-set in set"
        atslaDir  = directory "" "directory in labeledarray-element-set in set"
        atslaTrsf = spatialTransformation (0 0 1) (0 0 0) 1 "trsf in labeledarray-element-set in set"
    } ] *1 "labeledarray with comprehensive set elements in set"

} "set containing all parameter types"

<<<PARAMETERSET
*/


};

}

#endif
