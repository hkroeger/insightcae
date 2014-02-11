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
void copyPolyMesh(const boost::filesystem::path& from, const boost::filesystem::path& to, bool purify=false);

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

void mergeMeshes(const OpenFOAMCase& targetcase, const boost::filesystem::path& source, const boost::filesystem::path& target);

void resetMeshToLatestTimestep(const OpenFOAMCase& c, const boost::filesystem::path& location);

void runPvPython
(
  const OpenFOAMCase& ofc, 
  const boost::filesystem::path& location,
  const std::vector<std::string> pvpython_commands
);

}

#endif // OPENFOAMTOOLS_H
