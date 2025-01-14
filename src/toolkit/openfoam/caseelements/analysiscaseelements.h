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

#ifndef INSIGHT_ANALYSISCASEELEMENTS_H
#define INSIGHT_ANALYSISCASEELEMENTS_H

#include "openfoam/caseelements/openfoamcaseelement.h"
#include "base/resultset.h"
#include "base/analysis.h"

#include "analysiscaseelements__outputFilterFunctionObject__Parameters_headers.h"

namespace insight {


#ifndef SWIG
std::unique_ptr<std::pair<time_t,boost::filesystem::path> > newestOutputFile(
    const boost::filesystem::path& expectedFName
);
#endif

  
std::map<std::string,arma::mat> readAndCombineGroupedTabularFiles
(
    const OpenFOAMCase& cm, const boost::filesystem::path& caseLocation,
    const std::string& FOName, const std::string& fileName,
    int groupByColumn,
    const std::string& filterChars="()",
    const std::string& regionName = std::string()
);

arma::mat readAndCombineTabularFiles
    (
        const OpenFOAMCase& cm, const boost::filesystem::path& caseLocation,
        const std::string& FOName, const std::string& fileName,
        const std::string& filterChars="()",
        const std::string& regionName = std::string()
        );

/** ===========================================================================
 * Base class for function objects, that understand the output* options
 */
class outputFilterFunctionObject
: public OpenFOAMCaseElement
{
public:

#include "analysiscaseelements__outputFilterFunctionObject__Parameters.h"

/*
PARAMETERSET>>> outputFilterFunctionObject Parameters
inherits OpenFOAMCaseElement::Parameters

name = string "unnamed" "Name of the function object"
region = string "region0" "name of the region, defaults to the value of polyMesh::defaultRegion"
timeStart = double 0 "Time value, when the function object evaluation should start"
outputControl = string "outputTime" "Output time control"
outputInterval = double 1.0 "Time interval between outputs"

createGetter
<<<PARAMETERSET
*/

  
public:
  declareFactoryTable 
  ( 
      outputFilterFunctionObject, 
      LIST 
      (  
	  OpenFOAMCase& c, 
      ParameterSetInput&& ip
      ),
      LIST ( c, std::move(ip) )
  );
  declareStaticFunctionTable ( defaultParameters, std::unique_ptr<ParameterSet> );
  declareType("outputFilterFunctionObject");

  outputFilterFunctionObject(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
  virtual OFDictData::dict functionObjectDict() const =0;
  virtual std::vector<std::string> requiredLibraries() const;
  void addIntoControlDict(OFDictData::dict& controlDict) const;
  void addIntoDictionaries(OFdicts& dictionaries) const override;

  virtual void evaluate
  (
    OpenFOAMCase& cm, const boost::filesystem::path& location, ResultSetPtr& results, 
    const std::string& shortDescription
  ) const;
  
  static std::string category() { return "Postprocessing"; }
};




/** ===========================================================================
 * function object front end for field fieldAveraging
 */
class fieldAveraging
: public outputFilterFunctionObject
{
    
public:
#include "analysiscaseelements__fieldAveraging__Parameters.h"

/*
PARAMETERSET>>> fieldAveraging Parameters
inherits outputFilterFunctionObject::Parameters

fields = array [ string "U" "Name of a field" ]*1 "Names of fields to average in time"

createGetter
<<<PARAMETERSET
*/

  
public:
  declareType("fieldAveraging");
  fieldAveraging(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );

  OFDictData::dict functionObjectDict() const override;
};




/** ===========================================================================
 * function object front end for probes
 */
class probes
: public outputFilterFunctionObject
{
    
public:
#include "analysiscaseelements__probes__Parameters.h"

/*
PARAMETERSET>>> probes Parameters
inherits outputFilterFunctionObject::Parameters

fields = array [ string "U" "Name of a field" ]*1 "Names of fields to average in time"
probeLocations = array [ vector (0 0 0) "Probe point location" ]*1 "Locations of probe points"

createGetter
<<<PARAMETERSET
*/

  
public:
    declareType("probes");
    probes(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );

    /**
     * reads and returns probe sample data.
     * Matrix dimensions: [ninstants, npts, ncmpt]
     */
    static arma::cube readProbes
    (
        const OpenFOAMCase& c, 
        const boost::filesystem::path& location, 
        const std::string& foName, 
        const std::string& fieldName
    );
    
    /**
     * read sample locations from controlDict
     */
    static arma::mat readProbesLocations
    (
        const OpenFOAMCase& c, 
        const boost::filesystem::path& location, 
        const std::string& foName
    );
    static std::string category() {
        return "Postprocessing";
    }


    OFDictData::dict functionObjectDict() const override;
};




/** ===========================================================================
 * function object front end for volume integrate
 */
class volumeIntegrate
: public outputFilterFunctionObject
{

public:
#include "analysiscaseelements__volumeIntegrate__Parameters.h"

/*
PARAMETERSET>>> volumeIntegrate Parameters
inherits outputFilterFunctionObject::Parameters

fields = array [ string "alpha.phase1" "Name of a field" ]*1 "Names of fields to process over volume"
domain = selectablesubset {{

 wholedomain set { }

 cellZone set {
  cellZoneName = string "zone" "name of cellZone to integrate"
 }

}} wholedomain "select domain of integration"

weightFieldName = string "none" "Name of field for weighting. This is ignored, if no operation with weighting is selected."

operation = selection ( sum sumMag average volAverage volIntegrate min max CoV weightedVolIntegrate) volIntegrate "operation to execute on data"

createGetter
<<<PARAMETERSET
*/

public:
    declareType("volumeIntegrate");
    volumeIntegrate(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );

    static arma::mat readVolumeIntegrals
    (
        const OpenFOAMCase& c,
        const boost::filesystem::path& location,
        const std::string& FOName,
        const std::string& regionName = std::string()
    );

    static std::string category() {
        return "Postprocessing";
    }


    OFDictData::dict functionObjectDict() const override;
};


/** ===========================================================================
 * function object front end for surface integrate
 */
class surfaceIntegrate
: public outputFilterFunctionObject
{

public:
#include "analysiscaseelements__surfaceIntegrate__Parameters.h"

/*
PARAMETERSET>>> surfaceIntegrate Parameters
inherits outputFilterFunctionObject::Parameters

fields = array [ string "alpha.phase1" "Name of a field" ]*1 "Names of fields to average over volume"
domain = selectablesubset {{

 patch set {
  patchName = string "inlet" "name of patch to integrate"
 }

 faceZone set {
  faceZoneName = string "inlet" "name of faceZone to integrate"
 }

}} patch "select domain of integration"

operation = selection ( sum areaIntegrate areaAverage ) areaIntegrate "operation to execute on data"

createGetter
<<<PARAMETERSET
*/


public:
    declareType("surfaceIntegrate");
    surfaceIntegrate(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );

    static std::string category() {
        return "Postprocessing";
    }

    OFDictData::dict functionObjectDict() const override;

    static arma::mat readSurfaceIntegrate
    (
        const OpenFOAMCase& c,
        const boost::filesystem::path& location,
        const std::string& foName,
        const std::string& regionName = ""
    );
};







/** ===========================================================================
 * function object front end for volume integrate
 */
class fieldMinMax
: public outputFilterFunctionObject
{

public:
#include "analysiscaseelements__fieldMinMax__Parameters.h"

/*
PARAMETERSET>>> fieldMinMax Parameters
inherits outputFilterFunctionObject::Parameters

fields = array [ string "U" "Name of a field" ]*1 "Names of fields for which minima and maxima will be reported"

createGetter
<<<PARAMETERSET
*/


public:
    declareType("fieldMinMax");
    fieldMinMax(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );

    OFDictData::dict functionObjectDict() const override;

    static std::string category() {
        return "Postprocessing";
    }

    static std::map<std::string,arma::mat> readOutput
    (
        const OpenFOAMCase& c,
        const boost::filesystem::path& location,
        const std::string& foName,
        const std::string& regionName = std::string()
    );
};


/** ===========================================================================
 * function object front end for live cutting plane sampling during run
 */
class cuttingPlane
: public outputFilterFunctionObject
{
    
public:
#include "analysiscaseelements__cuttingPlane__Parameters.h"

/*
PARAMETERSET>>> cuttingPlane Parameters
inherits outputFilterFunctionObject::Parameters

fields = array [ string "U" "Name of a field" ]*1 "Names of fields to average in time"
basePoint = vector (0 0 0) "Cutting plane base point"
normal = vector (0 0 1) "Cutting plane normal direction"
interpolate = bool true "Whether to output VTKs with interpolated fields"

createGetter
<<<PARAMETERSET
*/
  
public:
  declareType("cuttingPlane");
  cuttingPlane(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );

  OFDictData::dict functionObjectDict() const override;
};




/** ===========================================================================
 * function object front end for two-point correlation sampling
 */
class twoPointCorrelation
: public outputFilterFunctionObject
{
public:
#include "analysiscaseelements__twoPointCorrelation__Parameters.h"

/*
PARAMETERSET>>> twoPointCorrelation Parameters
inherits outputFilterFunctionObject::Parameters

p0 = vector (0 0 0) "Base point"
directionSpan = vector (1 0 0) "Direction and distance of correlation"
homogeneousTranslationUnit = vector (0 1 0) "Translational distance between two subsequent correlation lines for homogeneous averaging"
np = int 50 "Number of correlation points"
nph = int 1 "Number of homogeneous averaging locations"

createGetter
<<<PARAMETERSET
*/

  
public:
  declareType("twoPointCorrelation");
  twoPointCorrelation(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
    

  OFDictData::dict functionObjectDict() const override;
  virtual OFDictData::dict csysConfiguration() const;

  static boost::ptr_vector<arma::mat> readCorrelations(const OpenFOAMCase& c, const boost::filesystem::path& location, const std::string& tpcName);
};




/** ===========================================================================
 * front end for sampling of two-point correlations in cylindrical CS
 */
class cylindricalTwoPointCorrelation
: public twoPointCorrelation
{
    
public:
#include "analysiscaseelements__cylindricalTwoPointCorrelation__Parameters.h"

/*
PARAMETERSET>>> cylindricalTwoPointCorrelation Parameters
inherits twoPointCorrelation::Parameters

ez = vector (0 0 1) "Axial direction"
er = vector (1 0 0) "Radial direction"
degrees = bool false ""

createGetter
<<<PARAMETERSET
*/

  
public:
  declareType("cylindricalTwoPointCorrelation");
  cylindricalTwoPointCorrelation(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );

  OFDictData::dict csysConfiguration() const override;
};



#ifdef SWIG
%template(getforcesCaseElement) insight::OpenFOAMCase::get<const forces>;
#endif

class forces
: public outputFilterFunctionObject
{
    
public:
#include "analysiscaseelements__forces__Parameters.h"

/*
PARAMETERSET>>> forces Parameters
inherits outputFilterFunctionObject::Parameters

patches = array [ string "patch" "Name of patch" ] *1 "Name of patches for force integration"
pName = string "p" "Name of pressure field"
UName = string "U" "Name of velocity field"
rhoName = string "rhoInf" "Name of density field. rhoInf for constant rho"
rhoInf = double 1.0 "Value of constant density"
CofR = vector (0 0 0) "Center point for torque calculation"

createGetter
<<<PARAMETERSET
*/

  
public:
  declareType("forces");
  forces(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );

  OFDictData::dict functionObjectDict() const override;
  std::vector<std::string> requiredLibraries() const override;
  
  static arma::mat readForces(const OpenFOAMCase& c, const boost::filesystem::path& location, const std::string& foName);
  
  static std::string category() { return "Postprocessing"; }
};





class extendedForces
: public forces
{
public:
#include "analysiscaseelements__extendedForces__Parameters.h"

/*
PARAMETERSET>>> extendedForces Parameters
inherits insight::forces::Parameters

maskField = string "" "Optional: name of field which masks the force evaluation. The local force density is multiplied by this field."
maskThreshold = double 0.5 "Threshold value for masking"

createGetter
<<<PARAMETERSET
*/

  
public:
  declareType("extendedForces");
  extendedForces(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
  OFDictData::dict functionObjectDict() const override;
  std::vector<std::string> requiredLibraries() const override;
};




class catalyst
: public OpenFOAMCaseElement
{
public:
#include "analysiscaseelements__catalyst__Parameters.h"

/*
PARAMETERSET>>> catalyst Parameters
inherits OpenFOAMCaseElement::Parameters

inputname = string "input" "Name of the input data set"

fields = array [ string "U" "Name of a field" ]*1 "Names of fields to export for insitu visualization"

scripts = array [ selectablesubset {{

   copy set {
    filename = path "" "The path to a catalyst pipeline script, which will be copied into the current case. This script can be exported from Paraview using the CatalystScriptGeneratorPlugin."
   }

   generate set {
    name = string "catalyst_pipeline" "Name of the generated script file (will be placed in FOAM_SRC/system)"
    extractBlock = bool true "Include an extractBlock filter"
    slice = bool false "Include a slice filter"
    contour = bool false "Include a contour filter"
   }

  }} generate "Select the source of the visualization pipeline script"

] *1 "Specifies the source of the one or more pipeline scripts"

paraview_host = string "localhost" "Name or IP of the host, where the Paraview client is running"
paraview_port = int 22222 "The port, where paraview is listening"

createGetter
<<<PARAMETERSET
*/


public:
  declareType("catalyst");
  catalyst(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
  static std::string category() { return "Postprocessing"; }
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  void modifyCaseOnDisk ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const override;
};




class CorrelationFunctionModel
: public RegressionModel
{
public:
  double A_, B_, omega_, phi_;
  
  CorrelationFunctionModel();
  int numP() const override;
  void setParameters(const double* params) override;
  void setInitialValues(double* params) const override;
  arma::mat weights(const arma::mat& x) const override;
  arma::mat evaluateObjective(const arma::mat& x) const override;
  
  double lengthScale() const;
};



class ComputeLengthScale
: public AnalysisWithParameters
{
public:
#include "analysiscaseelements__ComputeLengthScale__Parameters.h"

/*
PARAMETERSET>>> ComputeLengthScale Parameters

R_vs_x = matrix 10x2 "autocorrelation function: first column contains coordinate and second column the normalized autocorrelation values."

<<<PARAMETERSET
*/

    typedef supplementedInputDataDerived<Parameters> supplementedInputData;
    addParameterMembers_SupplementedInputData(ComputeLengthScale::Parameters);

public:
  declareType("ComputeLengthScale");

  ComputeLengthScale(
      const std::shared_ptr<supplementedInputDataBase>& sp );

  ResultSetPtr operator()(ProgressDisplayer& displayer=consoleProgressDisplayer) override;

  static std::string category() { return "General Postprocessing"; }
  static AnalysisDescription description() { return {"Length Scale", "Compute the length scale from autocorrelation functions"}; }
};



template<class TPC, const char* TypeName>
class TPCArray
: public outputFilterFunctionObject
{
  
  static const char * cmptNames[];
  
public:
#include "analysiscaseelements__TPCArrayBase__Parameters.h"

// x = double 0.0 "X-coordinate of base point"
// z = double 0.0 "Z-coordinate of base point"
// name_prefix = string "tpc" "name prefix of individual autocorrelation sampling FOs"
// timeStart = double 0.0 "start time of sampling. A decent UMean field has to be available in database at that time."
// outputControl = string "outputTime" "output control selection"
// outputInterval = double 10.0 "output interval"
  
/*
PARAMETERSET>>> TPCArrayBase Parameters
inherits outputFilterFunctionObject::Parameters

p0 = vector (0 0 0) "base point of TPC array"
tanSpan = double 3.14159265359 "length evaluation section in transverse direction"
axSpan = double 1.0 "length evaluation section in longitudinal direction"
np = int 50 "number of autocorrelation points"
nph = int 8 "number of instances for averaging over homogeneous direction"
R = double 1.0 "Maximum radius coordinate (or Y-coordinate) array. Minimum of range is always 0. Sampling locations are more concentrated at end point according to cosinus rule."
e_ax = vector (1 0 0) "longitudinal direction"
e_rad = vector (0 1 0) "radial direction"
e_tan = vector (0 0 1) "transverse direction"
grading = selection (towardsEnd towardsStart none) towardsEnd "Grading in placement of TPC sample FOs"

createGetter
<<<PARAMETERSET
*/

protected:
  std::vector<double> r_;
  boost::ptr_vector<TPC> tpc_ax_;
  boost::ptr_vector<TPC> tpc_tan_;
  
  typename TPC::Parameters getTanParameters(int i) const;
  typename TPC::Parameters getAxParameters(int i) const;
  
  std::string axisTitleTan() const;
  std::string axisTitleAx() const;
  
public:
  declareType(TypeName);
  
  TPCArray(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
  virtual OFDictData::dict functionObjectDict() const;
  void addIntoDictionaries(OFdicts& dictionaries) const override;
  virtual void evaluate(OpenFOAMCase& cm, const boost::filesystem::path& location, ResultSetPtr& results,
                        const std::string& shortDescription) const;
  virtual void evaluateSingle
  (
    OpenFOAMCase& cm, const boost::filesystem::path& location, 
    ResultSetPtr& results, 
    const std::string& name_prefix,
    double span,
    const std::string& axisLabel,
    const boost::ptr_vector<TPC>& tpcarray,
    const std::string& shortDescription,
    Ordering& so
  ) const;


};

extern const char LinearTPCArrayTypeName[];
typedef TPCArray<twoPointCorrelation, LinearTPCArrayTypeName> LinearTPCArray;
extern const char RadialTPCArrayTypeName[];
typedef TPCArray<cylindricalTwoPointCorrelation, RadialTPCArrayTypeName> RadialTPCArray;




}

#include "analysiscaseelementstemplates.h"

#endif // INSIGHT_ANALYSISCASEELEMENTS_H
