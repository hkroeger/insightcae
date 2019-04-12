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
#include "openfoam/openfoamcase.h"
#include "progrock/cppx/collections/options_boosted.h"


#ifdef SWIG
%template(TimeDirectoryList) std::map<double, boost::filesystem::path>;
#endif

namespace insight
{
  
typedef std::map<double, boost::filesystem::path> TimeDirectoryList;


TimeDirectoryList listTimeDirectories(const boost::filesystem::path& dir);

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
void linkPolyMesh(const boost::filesystem::path& from, const boost::filesystem::path& to, const OFEnvironment* env=NULL);

/*
 * Copy field files below "from" into "to"
 * "to" is created, if nonexistent
 */
void copyFields(const boost::filesystem::path& from, const boost::filesystem::path& to);

#ifndef SWIG
namespace setFieldOps
{
  
//typedef boost::tuple<std::string, std::string, FieldValue> FieldValueSpec;
typedef std::string FieldValueSpec;
typedef std::vector<FieldValueSpec> FieldValueSpecList;

class setFieldOperator
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
      ( fieldValues, FieldValueSpecList, FieldValueSpecList() )
  )

protected:
  Parameters p_;

public:
  setFieldOperator(Parameters const& p = Parameters() );
  virtual ~setFieldOperator();
  
  virtual void addIntoDictionary(OFDictData::dict& setFieldDict) const =0;
  
  virtual setFieldOperator* clone() const =0;
};


class fieldToCellOperator
: public setFieldOperator
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, setFieldOperator::Parameters,
      ( fieldName, std::string, "" )
      ( min, double, 0.0 )
      ( max, double, DBL_MAX )
  )

protected:
  Parameters p_;

public:
  fieldToCellOperator(Parameters const& p = Parameters() );
  
  virtual void addIntoDictionary(OFDictData::dict& setFieldDict) const;
  
  setFieldOperator* clone() const;
};

class boxToCellOperator
: public setFieldOperator
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, setFieldOperator::Parameters,
      ( min, arma::mat, vec3(-1e10, -1e10, -1e10) )
      ( max, arma::mat, vec3(1e10, 1e10, 1e10) )
  )

protected:
  Parameters p_;

public:
  boxToCellOperator(Parameters const& p = Parameters() );
  
  virtual void addIntoDictionary(OFDictData::dict& setFieldDict) const;
  
  setFieldOperator* clone() const;
};


class cellToCellOperator
: public setFieldOperator
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, setFieldOperator::Parameters,
      ( cellSet, std::string, "cellSet" )
  )

protected:
  Parameters p_;

public:
  cellToCellOperator(Parameters const& p = Parameters() );

  virtual void addIntoDictionary(OFDictData::dict& setFieldDict) const;

  setFieldOperator* clone() const;
};

inline setFieldOperator* new_clone(const setFieldOperator& op)
{
  return op.clone();
}

}

void setFields(const OpenFOAMCase& ofc, 
	       const boost::filesystem::path& location, 
	       const std::vector<setFieldOps::FieldValueSpec>& defaultValues,
	       const boost::ptr_vector<setFieldOps::setFieldOperator>& ops);

#endif


#ifndef SWIG
namespace createPatchOps
{
  

class createPatchOperator
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
      ( name, std::string, std::string("newpatch") )
      ( constructFrom, std::string, std::string("patches") )
      ( type, std::string, std::string("patch") )
      ( patches, std::vector<std::string>, std::vector<std::string>() )
      ( set, std::string, std::string("set") )
  )

protected:
  Parameters p_;

public:
  createPatchOperator(Parameters const& p = Parameters() );
  virtual ~createPatchOperator();
  
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& createPatchDict) const;
  virtual createPatchOperator* clone() const;
};

/**
 * Creates a cyclic patch or cyclic patch pair (depending on OF version)
 * from two other patches
 */
class createCyclicOperator
: public createPatchOperator
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, createPatchOperator::Parameters,
      ( name, std::string, std::string("newpatch") )
      ( constructFrom, std::string, std::string("patches") )
      ( patches_half1, std::vector<std::string>, std::vector<std::string>() )
      ( set_half1, std::string, std::string("set_half1") )
  )

protected:
  Parameters p_;

public:
  createCyclicOperator(Parameters const& p = Parameters() );
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& createPatchDict) const;
  virtual createPatchOperator* clone() const;
};

inline createPatchOperator* new_clone(const createPatchOperator& op)
{
  return op.clone();
}

}

void createPatch(const OpenFOAMCase& ofc, 
		  const boost::filesystem::path& location, 
		  const boost::ptr_vector<createPatchOps::createPatchOperator>& ops,
		  bool overwrite=true
		);
#endif



namespace sampleOps
{

class set
{

  
public:
#include "openfoamtools__sampleOps_set__Parameters.h"
/*
PARAMETERSET>>> sampleOps_set Parameters

name = string "unnamed" "Name of the set"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  set(ParameterSet const& p = Parameters::makeDefault() );
  virtual ~set();
  
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const =0;
  
  inline const std::string& name() const { return p_.name; }
  
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  
  virtual set* clone() const =0;
};

#ifndef SWIG
inline set* new_clone(const set& op)
{
  return op.clone();
}
#endif

/**
 * Creates a cyclic patch or cyclic patch pair (depending on OF version)
 * from two other patches
 */
struct ColumnInfo
{
  size_t col, ncmpt;
};

//typedef std::map<std::string, ColumnInfo > ColumnDescription;
class ColumnDescription
: public std::map<std::string, ColumnInfo >
{
public:
    inline bool contains(const std::string& name) const { return this->find(name) != this->end(); }
    inline long int colIndex(const std::string& name) const
    { 
        auto it = this->find(name);
        if (it != this->end() ) 
            return it->second.col;
        else
            return -1; 
    }
};

class line
: public set
{
public:
#include "openfoamtools__sampleOps_line__Parameters.h"
/*
PARAMETERSET>>> sampleOps_line Parameters
inherits set::Parameters

points = vector (0 0 0) "Matrix of points"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  line(ParameterSet const& p = Parameters::makeDefault() );
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual set* clone() const;
  
  /**
   * reads the sampled data from the files
   * OF writes different files for scalars, vectors tensors. 
   * They are all read and combined into a single matrix in the above order by column.
   * Only the last results in the last time folder is returned
   */
  arma::mat readSamples(const OpenFOAMCase& ofc, const boost::filesystem::path& location,
			       ColumnDescription* coldescr=NULL,
			       const std::string& time="" // empty string means latest
			      ) const;
};

class uniformLine
: public set
{
public:
#include "openfoamtools__sampleOps_uniformLine__Parameters.h"
/*
PARAMETERSET>>> sampleOps_uniformLine Parameters
inherits set::Parameters

start = vector (0 0 0) "Start point of line"
end = vector (0 0 0) "End point of line"
np = int 100 "Number of points"

<<<PARAMETERSET
*/

protected:
  Parameters p_;
  line l_;

public:
  uniformLine(ParameterSet const& p = Parameters::makeDefault() );
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual set* clone() const;
  
  /**
   * reads the sampled data from the files
   * OF writes different files for scalars, vectors tensors. 
   * They are all read and combined into a single matrix in the above order by column.
   * Only the last results in the last time folder is returned
   */
  arma::mat readSamples(const OpenFOAMCase& ofc, const boost::filesystem::path& location,
			       ColumnDescription* coldescr=NULL,
			       const std::string& time="" // empty string means latest
			      ) const;
};

class circumferentialAveragedUniformLine
: public set
{
public:
public:
#include "openfoamtools__sampleOps_circumferentialAveragedUniformLine__Parameters.h"
/*
PARAMETERSET>>> sampleOps_circumferentialAveragedUniformLine Parameters
inherits set::Parameters

start = vector (0 0 0) "Start point of line"
end = vector (1 0 0) "End point of line"
axis = vector (0 0 1) "Axis of rotation for circumferential averaging"
angle= double 6.28318530718 "Angular span for circumferential averaging"
angularOffset = double 0 "Angle from radial axis to first profile"

np = int 100 "Number of points along line"
nc = int 100 "Number of line for homogeneous averaging"

<<<PARAMETERSET
*/

protected:
  Parameters p_;
  double L_;
  arma::mat x_, dir_;
  boost::ptr_vector<line> lines_;

public:
  circumferentialAveragedUniformLine(ParameterSet const& p = Parameters::makeDefault() );
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual set* clone() const;
  
  arma::mat rotMatrix(int i, double angularOffset=0) const;
  inline std::string setname(int i) const { return p_.name+"-"+boost::lexical_cast<std::string>(i); }
  arma::mat readSamples(const OpenFOAMCase& ofc, const boost::filesystem::path& location, 
			       ColumnDescription* coldescr=NULL,
			       const std::string& time="" // empty string means latest
			      ) const;
};

class linearAveragedPolyLine
: public set
{
public:
#include "openfoamtools__sampleOps_linearAveragedPolyLine__Parameters.h"
/*
PARAMETERSET>>> sampleOps_linearAveragedPolyLine Parameters
inherits set::Parameters

points = vector (0 0 0) "Matrix of sample points"
dir1=vector (1 0 0) "direction 1, defines spacing between profiles in direction 1"
dir2=vector (0 0 1) "direction 2, defines spacing between profiles in direction 2"
nd1 = int 10 "Number of profiles along dir1"
nd2 = int 10 "Number of profiles along dir2"

<<<PARAMETERSET
*/

protected:
  linearAveragedPolyLine::Parameters p_;
  arma::mat x_;
  boost::ptr_vector<line> lines_;

public:
  linearAveragedPolyLine(ParameterSet const& p = Parameters::makeDefault() );
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual set* clone() const;
  
  inline std::string setname(int i, int j) const { return p_.name+"-"+boost::lexical_cast<std::string>(i*p_.nd1+j); }
  arma::mat readSamples(const OpenFOAMCase& ofc, const boost::filesystem::path& location, 
			       ColumnDescription* coldescr=NULL,
			       const std::string& time="" // empty string means latest
			      ) const;
};

class linearAveragedUniformLine
: public set
{
  
public:
#include "openfoamtools__sampleOps_linearAveragedUniformLine__Parameters.h"
/*
PARAMETERSET>>> sampleOps_linearAveragedUniformLine Parameters
inherits set::Parameters

start = vector (0 0 0) "start point of base profile"
end = vector (1 0 0) "end point of base profile"
np = int 100 "Number of sample points on base profile"

dir1=vector (1 0 0) "direction 1, defines spacing between profiles in direction 1"
dir2=vector (0 0 1) "direction 2, defines spacing between profiles in direction 2"
nd1 = int 10 "Number of profiles along dir1"
nd2 = int 10 "Number of profiles along dir2"

<<<PARAMETERSET
*/

protected:
  linearAveragedUniformLine::Parameters p_;
  linearAveragedPolyLine pl_;

public:
  linearAveragedUniformLine(ParameterSet const& p = Parameters::makeDefault() );
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual set* clone() const;
  
  arma::mat readSamples(const OpenFOAMCase& ofc, const boost::filesystem::path& location, 
			       ColumnDescription* coldescr=NULL,
			       const std::string& time="" // empty string means latest
			      ) const;
};

template<class T>
const T& findSet(const boost::ptr_vector<sampleOps::set>& sets, const std::string& name)
{
  const T* ptr=NULL;
  for (const set& s: sets)
  {
    if (s.name()==name)
    {
      ptr=dynamic_cast<const T*>(&s);
      if (ptr!=NULL) return *ptr;
    }
  }
  insight::Exception("Could not find a set with name "+name+" matching the requested type!");
  return *ptr;
}

}

void sample(const OpenFOAMCase& ofc, 
	    const boost::filesystem::path& location, 
	    const std::vector<std::string>& fields,
	    const boost::ptr_vector<sampleOps::set>& sets,
	    std::vector<std::string> addopts = boost::assign::list_of<std::string>("-latestTime")
	    );

// #endif 

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

void runPotentialFoam(const OpenFOAMCase& cm, const boost::filesystem::path& location, bool* stopFlagPtr=NULL, int np=1);

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
  patchIntegrate(const OpenFOAMCase& cm, const boost::filesystem::path& location,
		    const std::string& fieldName, const std::string& patchName,
		    const std::vector<std::string>& addopts=boost::assign::list_of<std::string>("-latestTime")
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

arma::mat readParaviewCSV(const boost::filesystem::path& file, std::map<std::string, int>* headers);
std::vector<arma::mat> readParaviewCSVs(const boost::filesystem::path& filetemplate, std::map<std::string, int>* headers);

std::string readSolverName(const boost::filesystem::path& ofcloc);
int readDecomposeParDict(const boost::filesystem::path& ofcloc);
std::string readTurbulenceModelName(const OpenFOAMCase& c, const boost::filesystem::path& ofcloc);

void meshQualityReport(const OpenFOAMCase& cm, 
		       const boost::filesystem::path& location, 
		       ResultSetPtr results,
		       const std::vector<std::string>& addopts = boost::assign::list_of<std::string>("-latestTime")
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


typedef std::map<std::string, int> PatchLayers;

void createPrismLayers
(
  const OpenFOAMCase& cm,
  const boost::filesystem::path& casePath,
  double finalLayerThickness, 
  bool relativeSizes, 
  const PatchLayers& nLayers,
  double expRatio,
  bool twodForExtrusion=false,
  bool isalreadydecomposed=false,
  bool keepdecomposedafterfinish=false
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
: public Analysis
{
public:
#include "openfoamtools__HomogeneousAveragedProfile__Parameters.h"
/*
PARAMETERSET>>> HomogeneousAveragedProfile Parameters

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

public:
  declareType("HomogeneousAveragedProfile");

  HomogeneousAveragedProfile(const ParameterSet& p, const boost::filesystem::path& exepath);

  static ParameterSet defaultParameters();
  static std::string category() { return "General Postprocessing"; }
  
  virtual ResultSetPtr operator()(ProgressDisplayer* displayer=NULL);
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

bool checkIfReconstructLatestTimestepNeeded
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location
);

typedef std::vector<arma::mat> EMeshPtsList;
typedef std::vector<EMeshPtsList> EMeshPtsListList;

void exportEMesh(const EMeshPtsList& pts, const boost::filesystem::path& filename);
void exportEMesh(const EMeshPtsListList& pts, const boost::filesystem::path& filename);


class OpenFOAMCaseDirs
{

  boost::filesystem::path location_;
  std::set<boost::filesystem::path> sysDirs_, postDirs_, procDirs_;
  std::vector<boost::filesystem::path> timeDirs_;

public:
  enum TimeDirOpt { All, OnlyFirst, OnlyLast, OnlyFirstAndLast, ExceptFirst };

public:
  OpenFOAMCaseDirs
  (
    const OpenFOAMCase& cm,
    const boost::filesystem::path& location
  );

  std::set<boost::filesystem::path> timeDirs( TimeDirOpt td = TimeDirOpt::All );

  std::set<boost::filesystem::path> caseFilesAndDirs
  (
      TimeDirOpt td = TimeDirOpt::All,
      bool cleanProc=true,
      bool cleanTimes=true,
      bool cleanPost=true,
      bool cleanSys=true
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
      bool cleanSys=true
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


}

#endif // OPENFOAMTOOLS_H
