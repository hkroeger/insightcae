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

#ifndef OPENFOAMTOOLS_H
#define OPENFOAMTOOLS_H

#include <string>
#include <vector>
#include <float.h>

#include "base/boost_include.h"

#include "base/analysis.h"
#include "base/resultset.h"
#include "base/tools.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/sampling.h"

#include "base/parameters/simpleparameter.h"
#include "base/supplementedinputdata.h"

#ifdef SWIG
%template(TimeDirectoryList) std::map<double, boost::filesystem::path>;
#endif

namespace insight
{
  
typedef std::map<double, boost::filesystem::path> TimeDirectoryList;


TimeDirectoryList listTimeDirectories(const boost::filesystem::path& dir, const boost::filesystem::path& fileInsideToAppend = "");

// std::string for compat with SWIG wrapper
std::string getLatestTimeDirectory(const boost::filesystem::path& dir);

std::string getOpenFOAMComponentLabel(int i, int ncmpt);
  
void setSet(const OpenFOAMCase& ofc, const boost::filesystem::path& location, const std::vector<std::string>& cmds);

void setsToZones(const OpenFOAMCase& ofc, const boost::filesystem::path& location, bool noFlipMap=true);

/*
 * Copy polyMesh directory below "from" into "to"
 * "to" is created, if nonexistent
 * Copy only basic mesh description, if "purify" is set
 */
void copyPolyMesh(const boost::filesystem::path& from, const boost::filesystem::path& to, 
		  bool purify=false, bool ignoremissing=false, bool include_zones=false);

/**
 * create mesh as symbolic links from other case.
 * If env is given, create a case skeleton in target case
 */
void linkPolyMesh(const boost::filesystem::path& from, const boost::filesystem::path& to,
                  const OFEnvironment* env=NULL);

/*
 * Copy field files below "from" into "to"
 * "to" is created, if nonexistent
 */
void copyFields(const boost::filesystem::path& from, const boost::filesystem::path& to);





/**
 * Converts a pair of patches into a cyclic pair using createPatch.
 * The names of the two patches must be of the pattern (.*)_half[0,1]. 
 * Only the name prefix (in parantheses_) must be supplied as an argument.
 */
void convertPatchPairToCyclic
(
  const OpenFOAMCase& ofc,
  const boost::filesystem::path& location, 
  const std::string& namePrefix
);

void mergeMeshes(const OpenFOAMCase& targetcase, const boost::filesystem::path& source, const boost::filesystem::path& target);


void mapFields
(
  const OpenFOAMCase& targetcase, 
  const boost::filesystem::path& source, 
  const boost::filesystem::path& target, 
  bool parallelTarget=false,
  const std::vector<std::string>& fields = std::vector<std::string>()
);

void resetMeshToLatestTimestep
(
    const OpenFOAMCase& c, 
    const boost::filesystem::path& location, 
    bool ignoremissing=false, 
    bool include_zones=false, 
    bool is_parallel=false
);

void runPotentialFoam(const OpenFOAMCase& cm, const boost::filesystem::path& location, int np=1);

void runPvPython
(
  const OpenFOAMCase& ofc, 
  const boost::filesystem::path& location,
  const std::vector<std::string> pvpython_commands,
  bool keepScript = false
);


class patchIntegrate
{
public:
  patchIntegrate(
        const OpenFOAMCase& cm,
        const boost::filesystem::path& location,
        const std::string& fieldName,
        const std::string& patchNamePattern,
        const std::string& regionName = std::string(),
        const std::vector<std::string>& addopts = {"-latestTime"}
        );

  /**
   * @brief t_
   * time/iteration values for subsequent arrays
   */
  arma::mat t_;

  /**
   * @brief A_vs_t_
   * area for different times/iterations
   */
  arma::mat A_;

  /**
   * @brief int_vs_t_
   * integral values for different times/iterations
   */
  arma::mat integral_values_;

  size_t n() const;
};




class patchArea
{
public:
  patchArea(const OpenFOAMCase& cm, const boost::filesystem::path& location,
            const std::string& patchName);

  double A_;
  arma::mat n_;
  arma::mat ctr_;
};

arma::mat readTextFile(std::istream& is);

arma::mat readParaviewCSV(const boost::filesystem::path& file, std::map<std::string, int>* headers);
std::vector<arma::mat> readParaviewCSVs(const boost::filesystem::path& filetemplate, std::map<std::string, int>* headers);

std::string readSolverName(const boost::filesystem::path& ofcloc);
int readDecomposeParDict(const boost::filesystem::path& ofcloc);
std::string readTurbulenceModelName(const OpenFOAMCase& c, const boost::filesystem::path& ofcloc);

struct MeshQualityInfo
{
  std::string time;

  int ncells;
  int nhex, nprism, ntet, npoly;

  int nmeshregions;

  arma::mat bb_min, bb_max;
  double max_aspect_ratio;
  std::string min_faceA, min_cellV;

  double max_nonorth, avg_nonorth;
  int n_severe_nonorth;

  int n_neg_facepyr;

  double max_skewness;
  int n_severe_skew;

  MeshQualityInfo();
};

typedef std::vector<MeshQualityInfo> MeshQualityList;

MeshQualityList getMeshQuality(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& location,
    const std::vector<std::string>& addopts
    );

void meshQualityReport(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& location,
    ResultSetPtr results,
    const std::vector<std::string>& addopts = {"-latestTime"}
);

void currentNumericalSettingsReport(const OpenFOAMCase& cm, 
		       const boost::filesystem::path& location, 
		       ResultSetPtr results
		      );

arma::mat viscousForceProfile
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const arma::mat& axis, int n,
  const std::vector<std::string>& addopts = boost::assign::list_of<std::string>("-latestTime")
);

arma::mat projectedArea
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const arma::mat& direction,
  const std::vector<std::string>& patches,
  const std::vector<std::string>& addopts = boost::assign::list_of<std::string>("-latestTime") 
);

arma::mat minPatchPressure
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const std::string& patch,
  const double& Af=0.025,
  const std::vector<std::string>& addopts = boost::assign::list_of<std::string>("-latestTime") 
);

void surfaceFeatureExtract
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const std::string& surfaceName
);


/**
 * Converts a 3D mesh (e.g. generated by snappyHexMesh) into a 2D mesh
 * by just keeping the extrusion of a lateral patch
 * Assumptions:
 *  - X and Y directions are the 2D directions
 *  - mesh extent in Z direction is from 0 to some positive value
 *  - sourcePatchName is the patch lying in the XY plane
 *  - the 2D mesh is centered around the XY plane
 */
void extrude2DMesh
(
  const OpenFOAMCase& c, 
  const boost::filesystem::path& location, 
  const std::string& sourcePatchName,
  std::string sourcePatchName2="",
  bool wedgeInsteadOfPrism=false,
  double distance=1.0,
  const arma::mat& offsetTranslation=vec3(0,0,0.5),
  const arma::mat& fixedDirection = arma::mat()
);

/**
 * Rotates a lateral patch of a 3D mesh (e.g. generated by snappyHexMesh) into 
 * a full 3D cylindrical mesh
 * Assumptions:
 *  - sourcePatchName is the patch lying in a coordinate plane
 */
void rotateMesh
(
  const OpenFOAMCase& c, 
  const boost::filesystem::path& location, 
  const std::string& sourcePatchName,
  int nc,
  const arma::mat& axis,
  const arma::mat& p0
);

void createBaffles
(
  const OpenFOAMCase& c, 
  const boost::filesystem::path& location, 
  const std::string& faceZoneName
);


/**
 * return extrema in specified zone
 * @return pair of min and max, first col time, others one col per component
 */
std::pair<arma::mat, arma::mat> zoneExtrema
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const std::string fieldName,
  const std::string zoneName,
  const std::vector<std::string>& addopts = boost::assign::list_of<std::string>("-latestTime") 
);

void removeCellSetFromMesh
(
  const OpenFOAMCase& c, 
  const boost::filesystem::path& location, 
  const std::string& cellSetName
);

arma::mat interiorPressureFluctuationProfile
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const arma::mat& axis, int n,
  const std::vector<std::string>& addopts= boost::assign::list_of<std::string>("-latestTime")
);


std::set<std::string> readPatchNameList(
        const OpenFOAMCase& cm,
        const boost::filesystem::path& caseLocation,
        bool parallel,
        const std::string& regionName = std::string(),
        const std::string& time = "constant");

class PatchLayers
        : public std::map<std::string, int>
{
public:
    PatchLayers();
    PatchLayers(
            const OpenFOAMCase& cm,
            const boost::filesystem::path& caseLocation,
            bool parallel,
            const std::string& regionName = std::string(),
            const std::string& time = "constant" );

    void setByPattern(const std::string& regex_pattern, int nLayers);
};

void createPrismLayers
(
  const OpenFOAMCase& cm,
  const boost::filesystem::path& casePath,
  double finalLayerThickness, 
  bool relativeSizes, 
  const PatchLayers& nLayers,
  double expRatio,
  bool twodForExtrusion = false,
  bool isalreadydecomposed = false,
  bool keepdecomposedafterfinish = false,
  ProgressDisplayer* progress = nullptr
);

arma::mat surfaceProjectLine
(
  const OFEnvironment& ofe, 
  const boost::filesystem::path& surfaceFile,
  const arma::mat& start, const arma::mat& end, int npts,
  const arma::mat& projdir
);

std::vector<boost::filesystem::path> searchOFCasesBelow(const boost::filesystem::path& basepath);



class HomogeneousAveragedProfile
: public AnalysisWithParameters
{
public:
#include "openfoamtools__HomogeneousAveragedProfile__Parameters.h"
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


std::vector<std::string> patchList
(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& caseDir,
    const std::string& include=".*",
    const std::vector<std::string>& exclude = std::vector<std::string>() // OF syntax: either string or regex (in quotes)
);


void calcR
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const std::vector<std::string>& addopts= boost::assign::list_of<std::string>("-latestTime")
);

void calcLambda2
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const std::vector<std::string>& addopts= boost::assign::list_of<std::string>("-latestTime")
);

/**
 * check, if any file in orig is not existing in copy or newer in orig.
 */
bool checkIfAnyFileIsNewerOrNonexistent
(
    boost::filesystem::path orig,
    boost::filesystem::path copy,
    bool recursive=true
);



class ParallelTimeDirectories
{
  const OpenFOAMCase& cm_;
  boost::filesystem::path location_;
  TimeDirectoryList serTimes_;
  TimeDirectoryList proc0Times_;
  std::set<boost::filesystem::path> procDirs_;

public:
  ParallelTimeDirectories(
        const OpenFOAMCase& cm,
        const boost::filesystem::path& location
        );

   bool proc0TimeDirNeedsReconst(const boost::filesystem::path& ptdname) const;

    /**
   * @brief newParallelTimes
   * @return
   * parallel time directories, which are newer
   * or not present in serial and need reconstruction
   */
  std::set<boost::filesystem::path>
  newParallelTimes(bool filterOutInconsistent=false) const;

  /**
   * @brief latestTimeNeedsReconst
   * @return
   * check, if latest time step needs reconstruction
   */
  bool latestTimeNeedsReconst() const;

  bool isParallelTimeDirInconsistent(const boost::filesystem::path& timeDirName) const;

  /**
   * @brief reconstructNewTimeDirs
   * reconstructs all time directories in processors directories
   * which are not yet reconstructed
   */
  void reconstructNewTimeDirs() const;

};

bool checkIfReconstructLatestTimestepNeeded
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location
);



typedef std::vector<arma::mat> EMeshPtsList;
typedef std::vector<EMeshPtsList> EMeshPtsListList;


class eMesh
{
protected:
    std::vector<arma::mat> points_;
    std::vector<std::pair<int, int> > edges_;

public:
    eMesh();
    eMesh(const EMeshPtsList& pts);
    eMesh(const EMeshPtsListList& pts);
    int nPoints() const;
    int nEdges() const;
    void write(std::ostream& os) const;
    void write(const boost::filesystem::path& filename) const;
};

#ifndef SWIG
std::ostream &operator<<(std::ostream& os, const eMesh& emesh);
#endif

void exportEMesh(const EMeshPtsList& pts, const boost::filesystem::path& filename);
void exportEMesh(const EMeshPtsListList& pts, const boost::filesystem::path& filename);

#ifndef SWIG
std::ostream& operator<<(std::ostream& os, const std::set<boost::filesystem::path>& paths);
#endif

class OpenFOAMCaseDirs
{

  boost::filesystem::path location_;
  std::set<boost::filesystem::path> sysDirs_, postDirs_, procDirs_;
  std::vector<boost::filesystem::path> timeDirs_;
  std::set<boost::filesystem::path> procTimeDirs_;

public:
  enum TimeDirOpt { All, OnlyFirst, OnlyLast, OnlyFirstAndLast, ExceptFirst };

public:
  OpenFOAMCaseDirs
  (
    const OpenFOAMCase& cm,
    const boost::filesystem::path& location
  );

  std::set<boost::filesystem::path> timeDirs( TimeDirOpt td = TimeDirOpt::All );

  static std::set<boost::filesystem::path> timeDirContent(
          const boost::filesystem::path& td );


  std::set<boost::filesystem::path> caseFilesAndDirs
  (
      TimeDirOpt td = TimeDirOpt::All,
      bool cleanProc = true,
      bool cleanTimes = true,
      bool cleanPost = true,
      bool cleanSys = true,
      bool cleanInconsistentParallelTimes = false
  );

  void packCase(const boost::filesystem::path& archive_file, TimeDirOpt td = TimeDirOpt::All);

  /**
   * @brief cleanCase
   * Removes all remainings of an OpenFOAM case (constant, system, processor*, postProcessing) from location.
   * Return a list with all directories and files, which have been deleted.
   */
  void cleanCase
  (
      TimeDirOpt td = TimeDirOpt::All,
      bool cleanProc=true,
      bool cleanTimes=true,
      bool cleanPost=true,
      bool cleanSys=true,
      bool cleanInconsistentParallelTimes = false
  );
};

void VTKFieldToOpenFOAMField(const boost::filesystem::path& vtkfile, const std::string& fieldname, std::ostream& out);

struct decompositionState
{
  bool hasProcessorDirectories;
  bool nProcDirsMatchesDecomposeParDict;
  bool decomposedLatestTimeIsConsistent;
  enum Location { Reconstructed, Decomposed, Both, Undefined };
  Location laterLatestTime;
  Location newerFiles;

  decompositionState(const boost::filesystem::path& casedir);
};


class BoundingBox
    : public arma::mat
{
public:
  BoundingBox();
  void extend(const arma::mat& bb);

  void operator=(const arma::mat& bb);
};



void setHydrostaticPressure(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& caseDir,
    const arma::mat& pSurf,
    const arma::mat& eUp,
    double rho, double p0Amb,
    const std::map<std::string, std::string> &targetEntriesPerPatch = {},
    const std::string& fieldName = "p",
    bool setInternalField = true
    );


}

#endif // OPENFOAMTOOLS_H
