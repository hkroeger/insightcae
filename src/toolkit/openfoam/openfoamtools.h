/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2013  hannes <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef OPENFOAMTOOLS_H
#define OPENFOAMTOOLS_H

#include <string>
#include <vector>

#include "boost/filesystem.hpp"
#include "boost/assign.hpp"

#include "base/resultset.h"
#include "openfoam/openfoamcase.h"
#include "progrock/cppx/collections/options_boosted.h"

namespace insight
{
  
typedef std::map<double, boost::filesystem::path> TimeDirectoryList;

TimeDirectoryList listTimeDirectories(const boost::filesystem::path& dir);
  
void setSet(const OpenFOAMCase& ofc, const boost::filesystem::path& location, const std::vector<std::string>& cmds);

void setsToZones(const OpenFOAMCase& ofc, const boost::filesystem::path& location, bool noFlipMap=true);

/*
 * Copy polyMesh directory below "from" into "to"
 * "to" is created, if nonexistent
 * Copy only basic mesh description, if "purify" is set
 */
void copyPolyMesh(const boost::filesystem::path& from, const boost::filesystem::path& to, bool purify=false, bool ignoremissing=false);

void linkPolyMesh(const boost::filesystem::path& from, const boost::filesystem::path& to);

/*
 * Copy field files below "from" into "to"
 * "to" is created, if nonexistent
 */
void copyFields(const boost::filesystem::path& from, const boost::filesystem::path& to);

namespace setFieldOps
{
  
//typedef boost::tuple<std::string, std::string, FieldValue> FieldValueSpec;
typedef std::string FieldValueSpec;

class setFieldOperator
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
      ( fieldValues, std::vector<FieldValueSpec>, std::vector<FieldValueSpec>() )
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

typedef std::map<std::string, ColumnInfo > ColumnDescription;

class uniformLine
: public set
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, set::Parameters,
      ( start, arma::mat, vec3(0,0,0) )
      ( end, arma::mat, vec3(1,0,0) )
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
   * multiple times are combined by row
   */
  static arma::mat readSamples(const OpenFOAMCase& ofc, const boost::filesystem::path& location, 
			       const std::string& setName,
			       ColumnDescription* coldescr=NULL
			      );
};

class circumferentialAveragedUniformLine
: public set
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, set::Parameters,
      ( start, arma::mat, vec3(0,0,0) )
      ( end, arma::mat, vec3(1,0,0) )
      ( axis, arma::mat, vec3(1,0,0) )
      ( np, int, 100 )
      ( nc, int, 10 )
  )

protected:
  Parameters p_;

public:
  circumferentialAveragedUniformLine(Parameters const& p = Parameters() );
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
  virtual set* clone() const;
  
  arma::mat rotMatrix(int i) const;
  inline std::string setname(int i) const { return p_.name()+"-"+boost::lexical_cast<std::string>(i); }
  arma::mat readSamples(const OpenFOAMCase& ofc, const boost::filesystem::path& location, 
			       ColumnDescription* coldescr=NULL
			      ) const;
};

class linearAveragedUniformLine
: public set
{
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
  Parameters p_;
  arma::mat x_;

public:
  linearAveragedUniformLine(Parameters const& p = Parameters() );
  virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
  virtual set* clone() const;
  
  inline std::string setname(int i, int j) const { return p_.name()+"-"+boost::lexical_cast<std::string>(i*p_.nd1()+j); }
  arma::mat readSamples(const OpenFOAMCase& ofc, const boost::filesystem::path& location, 
			       ColumnDescription* coldescr=NULL
			      ) const;
};

}

void sample(const OpenFOAMCase& ofc, 
	    const boost::filesystem::path& location, 
	    const std::vector<std::string>& fields,
	    const boost::ptr_vector<sampleOps::set>& sets
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

void mapFields(const OpenFOAMCase& targetcase, const boost::filesystem::path& source, const boost::filesystem::path& target, bool parallelTarget=false);

void resetMeshToLatestTimestep(const OpenFOAMCase& c, const boost::filesystem::path& location, bool ignoremissing=false);

void runPotentialFoam(const OpenFOAMCase& cm, const boost::filesystem::path& location, bool* stopFlagPtr=NULL, int np=1);

void runPvPython
(
  const OpenFOAMCase& ofc, 
  const boost::filesystem::path& location,
  const std::vector<std::string> pvpython_commands
);

arma::mat patchIntegrate(const OpenFOAMCase& cm, const boost::filesystem::path& location,
		    const std::string& fieldName, const std::string& patchName,
		    const std::vector<std::string>& addopts=boost::assign::list_of<std::string>("-latestTime")
			);


arma::mat readParaviewCSV(const boost::filesystem::path& filetemplate, std::map<std::string, int>* headers=NULL, int num=-1);

int readDecomposeParDict(const boost::filesystem::path& ofcloc);

void meshQualityReport(const OpenFOAMCase& cm, 
		       const boost::filesystem::path& location, 
		       ResultSetPtr results,
		       const std::vector<std::string>& addopts = boost::assign::list_of<std::string>("-latestTime")
		      );

arma::mat viscousForceProfile
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const arma::mat& axis, int n,
  const std::vector<std::string>& addopts = boost::assign::list_of<std::string>("-latestTime")
);

}

#endif // OPENFOAMTOOLS_H
