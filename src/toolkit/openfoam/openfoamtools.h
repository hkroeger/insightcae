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

inline setFieldOperator* new_clone(const setFieldOperator& op)
{
  return op.clone();
}

}

void setFields(const OpenFOAMCase& ofc, 
	       const boost::filesystem::path& location, 
	       const std::vector<setFieldOps::FieldValueSpec>& defaultValues,
	       const boost::ptr_vector<setFieldOps::setFieldOperator>& ops);


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

namespace sampleOps
{

class set
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
      ( name, std::string, "unnamed" )
  )

protected:
  Parameters p_;

public:
  set(Parameters const& p = Parameters() );
  virtual ~set();
  
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const =0;
  
  inline const std::string& name() const { return p_.name(); }
  
  virtual set* clone() const =0;
};

inline set* new_clone(const set& op)
{
  return op.clone();
}

/**
 * Creates a cyclic patch or cyclic patch pair (depending on OF version)
 * from two other patches
 */
struct ColumnInfo
{
  int col, ncmpt;
};

//typedef std::map<std::string, ColumnInfo > ColumnDescription;
class ColumnDescription
: public std::map<std::string, ColumnInfo >
{
public:
    inline bool contains(const std::string& name) const { return this->find(name) != this->end(); }
    inline int colIndex(const std::string& name) const 
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
  CPPX_DEFINE_OPTIONCLASS(Parameters, set::Parameters,
      ( points, arma::mat, vec3(0,0,0) )
  )

protected:
  Parameters p_;

public:
  line(Parameters const& p = Parameters() );
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
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
  line l_;
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, set::Parameters,
      ( start, arma::mat, vec3(0,0,0) )
      ( end, arma::mat, vec3(0,0,0) )
      ( np, int, 100 )
  )

protected:
  Parameters p_;

public:
  uniformLine(Parameters const& p = Parameters() );
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
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
  CPPX_DEFINE_OPTIONCLASS(Parameters, set::Parameters,
      ( start, arma::mat, vec3(0,0,0) )
      ( end, arma::mat, vec3(1,0,0) )
      ( axis, arma::mat, vec3(1,0,0) )
      ( angle, double, 2.*M_PI )  // angular span for averaging
      ( angularOffset, double, 0.0 ) // angle from radial axis to first profile
      ( np, int, 100 )
      ( nc, int, 10 )
  )

protected:
  Parameters p_;
  double L_;
  arma::mat x_, dir_;
  boost::ptr_vector<line> lines_;

public:
  circumferentialAveragedUniformLine(Parameters const& p = Parameters() );
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
  virtual set* clone() const;
  
  arma::mat rotMatrix(int i, double angularOffset=0) const;
  inline std::string setname(int i) const { return p_.name()+"-"+boost::lexical_cast<std::string>(i); }
  arma::mat readSamples(const OpenFOAMCase& ofc, const boost::filesystem::path& location, 
			       ColumnDescription* coldescr=NULL,
			       const std::string& time="" // empty string means latest
			      ) const;
};

class linearAveragedPolyLine
: public set
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, set::Parameters,
      ( points, arma::mat, vec3(0,0,0) )
      ( dir1, arma::mat, vec3(1,0,0) )
      ( dir2, arma::mat, vec3(0,0,1) )
      ( nd1, int, 10 )
      ( nd2, int, 10 )
  )

protected:
  linearAveragedPolyLine::Parameters p_;
  arma::mat x_;
  boost::ptr_vector<line> lines_;

public:
  linearAveragedPolyLine(linearAveragedPolyLine::Parameters const& p = linearAveragedPolyLine::Parameters() );
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
  virtual set* clone() const;
  
  inline std::string setname(int i, int j) const { return p_.name()+"-"+boost::lexical_cast<std::string>(i*p_.nd1()+j); }
  arma::mat readSamples(const OpenFOAMCase& ofc, const boost::filesystem::path& location, 
			       ColumnDescription* coldescr=NULL,
			       const std::string& time="" // empty string means latest
			      ) const;
};

class linearAveragedUniformLine
: public set
{
  linearAveragedPolyLine pl_;
  
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, set::Parameters,
      ( start, arma::mat, vec3(0,0,0) )
      ( end, arma::mat, vec3(1,0,0) )
      ( np, int, 100 )
      ( dir1, arma::mat, vec3(1,0,0) )
      ( dir2, arma::mat, vec3(0,0,1) )
      ( nd1, int, 10 )
      ( nd2, int, 10 )
  )

protected:
  linearAveragedUniformLine::Parameters p_;

public:
  linearAveragedUniformLine(linearAveragedUniformLine::Parameters const& p = linearAveragedUniformLine::Parameters() );
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
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
  BOOST_FOREACH(const set& s, sets)
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
	    const std::vector<std::string>& addopts = boost::assign::list_of<std::string>("-latestTime")
	    );
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

void resetMeshToLatestTimestep(const OpenFOAMCase& c, const boost::filesystem::path& location, bool ignoremissing=false, bool include_zones=false);

void runPotentialFoam(const OpenFOAMCase& cm, const boost::filesystem::path& location, bool* stopFlagPtr=NULL, int np=1);

void runPvPython
(
  const OpenFOAMCase& ofc, 
  const boost::filesystem::path& location,
  const std::vector<std::string> pvpython_commands
);

namespace paraview
{
  
    
    
    
class PVScene
{
public:
//   declareFactoryTableNoArgs(PVScene);
  declareDynamicClass(PVScene);
  
public:
  
#include "openfoamtools__PVScene__Parameters.h"
/*
PARAMETERSET>>> PVScene Parameters

resetview = bool false "If true, the view is cleared before rendering. Previous scene will be overlayed otherwise"
imagename = string "" "Image name. Will be used as filename. If blank, the view created but not rendered. This can be useful to overlay with the next scene."

<<<PARAMETERSET
*/

  declareType("PVscene");
  
  virtual ~PVScene();
  
  virtual std::string pythonCommands() const =0;
};




class CustomPVScene
: public PVScene
{
  
public:
#include "openfoamtools__CustomPVScene__Parameters.h"
/*
PARAMETERSET>>> CustomPVScene Parameters

inherits insight::paraview::PVScene::Parameters

command = string "" "Python snippet to execute in pvBatch"

<<<PARAMETERSET
*/
protected:
  Parameters p_;

public:
  declareType("custom");
  
  CustomPVScene(const ParameterSet&);
  
  virtual std::string pythonCommands() const;
  
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual ParameterSet getParameters() const { return p_; }
};




class CutplanePVScene
: public PVScene
{
  
public:
#include "openfoamtools__CutplanePVScene__Parameters.h"
/*
PARAMETERSET>>> CutplanePVScene Parameters

inherits insight::paraview::PVScene::Parameters

dataset = string "" "name of the data set to cut"
field = string "" "name of the field to display on the plane"

p0 = vector (0 0 0) "center point of the cut plane"
normal = vector (0 0 1) "normal vector of the cut plane"
camera = vector (0 0 1) "location of the view camera (looking on p0)"
up = vector (0 1 0) "upward direction of the view"
size = double 1.0 "size of the viewport (in the same units as the data set)"

<<<PARAMETERSET
*/
protected:
  Parameters p_;
  
public:
  declareType("cutplane");
  
  CutplanePVScene(const ParameterSet&);
  
  virtual std::string pythonCommands() const;
  
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual ParameterSet getParameters() const { return p_; }
};




class ParaviewVisualization
: public Analysis
{
public:
#include "openfoamtools__ParaviewVisualization__Parameters.h"
/*
PARAMETERSET>>> ParaviewVisualization Parameters

scenes = array [
 dynamicclassconfig "paraview::PVScene" default "custom" "Scene configuration"
 ] *0 "Configuration of scenes"

<<<PARAMETERSET
*/

  declareType("ParaviewVisualization");

  ParaviewVisualization(const ParameterSet& ps, const boost::filesystem::path& exepath);

  static ParameterSet defaultParameters();
  static std::string category() { return "General Postprocessing"; }
  
  virtual ResultSetPtr operator()(ProgressDisplayer* displayer=NULL);
};




}




arma::mat patchIntegrate(const OpenFOAMCase& cm, const boost::filesystem::path& location,
		    const std::string& fieldName, const std::string& patchName,
		    const std::vector<std::string>& addopts=boost::assign::list_of<std::string>("-latestTime")
			);


arma::mat readParaviewCSV(const boost::filesystem::path& file, std::map<std::string, int>* headers);
std::vector<arma::mat> readParaviewCSVs(const boost::filesystem::path& filetemplate, std::map<std::string, int>* headers);

std::string readSolverName(const boost::filesystem::path& ofcloc);
int readDecomposeParDict(const boost::filesystem::path& ofcloc);
std::string readTurbulenceModelName(const boost::filesystem::path& ofcloc);

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
  double distance=1.0
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
  * return bounding box of model
  * first col: min point
  * second col: max point
  */
arma::mat STLBndBox
(
  const OFEnvironment& ofe, 
  const boost::filesystem::path& path
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


}

#endif // OPENFOAMTOOLS_H
