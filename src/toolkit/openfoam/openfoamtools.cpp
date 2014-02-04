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

#include "openfoamtools.h"

#include "boost/filesystem.hpp"
#include "boost/ptr_container/ptr_vector.hpp"
#include "boost/assign.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;

namespace insight
{
  
TimeDirectoryList listTimeDirectories(const boost::filesystem::path& dir)
{
  TimeDirectoryList list;
  if ( exists( dir ) ) 
  {
    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( dir );
          itr != end_itr;
          ++itr )
    {
      if ( is_directory(itr->status()) )
      {
        std::string fn=itr->path().filename().string();
	try
	{
	  double time = lexical_cast<double>(fn);
	  list[time]=itr->path();
	}
	catch (...)
	{
	}
      }
    }
  }
  return list;
}

  
void setSet(const OpenFOAMCase& ofc, const boost::filesystem::path& location, const std::vector<std::string>& cmds)
{
  redi::opstream proc;
  ofc.forkCommand(proc, location, "setSet");
  BOOST_FOREACH(const std::string& line, cmds)
  {
    proc << line << endl;
  }
  proc << "quit" << endl;
}

void setsToZones(const OpenFOAMCase& ofc, const boost::filesystem::path& location, bool noFlipMap)
{
  std::vector<std::string> args;
  if (noFlipMap) args.push_back("-noFlipMap");
  ofc.executeCommand(location, "setsToZones", args);
}

void copyPolyMesh(const boost::filesystem::path& from, const boost::filesystem::path& to, bool purify)
{
  path source(from/"polyMesh");
  path target(to/"polyMesh");
  if (!exists(target))
    create_directories(target);
  
  std::string cmd("ls "); cmd+=source.c_str();
  ::system(cmd.c_str());
  
  if (purify)
  {
    BOOST_FOREACH(const std::string& fname, 
		  list_of<std::string>("boundary")("faces")("neighbour")("owner")("points")
		  .convert_to_container<std::vector<std::string> >())
    {
      path gzname(fname.c_str()); gzname+=".gz";
      if (exists(source/gzname)) copy_file(source/gzname, target/gzname);
      else if (exists(source/fname)) copy_file(source/fname, target/fname);
      else throw insight::Exception("Essential mesh file "+fname+" not present in "+source.c_str());
    }
  }
  else
    throw insight::Exception("Not implemented!");
}


void copyFields(const boost::filesystem::path& from, const boost::filesystem::path& to)
{
  if (!exists(to))
    create_directories(to);

  directory_iterator end_itr; // default construction yields past-the-end
  for ( directory_iterator itr( from );
	itr != end_itr;
	++itr )
  {
    if ( is_regular_file(itr->status()) )
    {
      copy_file(itr->path(), to/itr->path().filename());
    }
  }
}

namespace setFieldOps
{
 
setFieldOperator::setFieldOperator(Parameters const& p)
: p_(p)
{
}

fieldToCellOperator::fieldToCellOperator(Parameters const& p)
: setFieldOperator(p),
  p_(p)
{
}
  
void fieldToCellOperator::addIntoDictionary(OFDictData::dict& setFieldDict) const
{
  OFDictData::dict opdict;
  opdict["fieldName"]=p_.fieldName();
  opdict["min"]=p_.min();
  opdict["max"]=p_.max();

  OFDictData::list fve;
  BOOST_FOREACH(const FieldValueSpec& fvs, p_.fieldValues())
  {
    //std::ostringstream line;
    //line << fvs.get<0>() << " " << fvs.get<1>() ;
    fve.push_back( fvs );
  }
  opdict["fieldValues"]=fve;
  setFieldDict.getList("regions").push_back( fve );
  
}

setFieldOperator* fieldToCellOperator::clone() const
{
  return new fieldToCellOperator(p_);
}

}

void setFields(const OpenFOAMCase& ofc, const boost::filesystem::path& location, 
	       const std::vector<setFieldOps::FieldValueSpec>& defaultValues,
	       const boost::ptr_vector<setFieldOps::setFieldOperator>& ops)
{
  using namespace setFieldOps;
  
  OFDictData::dict setFieldsDict;
  
  OFDictData::list& dvl = setFieldsDict.addListIfNonexistent("defaultFieldValues");
  BOOST_FOREACH( const FieldValueSpec& dv, defaultValues)
  {
    dvl.push_back( dv );
  }
  
  setFieldsDict.addListIfNonexistent("regions");  
  BOOST_FOREACH( const setFieldOperator& op, ops)
  {
    op.addIntoDictionary(setFieldsDict);
  }
  
  // then write to file
  boost::filesystem::path dictpath = location / "system" / "setFieldDict";
  if (!exists(dictpath.parent_path())) 
  {
    boost::filesystem::create_directories(dictpath.parent_path());
  }
  
  {
    std::ofstream f(dictpath.c_str());
    writeOpenFOAMDict(f, setFieldsDict, boost::filesystem::basename(dictpath));
  }

  ofc.executeCommand(location, "setField");
}

namespace createPatchOps
{

createPatchOperator::createPatchOperator(Parameters const& p )
: p_(p)
{
}
  
void createPatchOperator::addIntoDictionary(OFDictData::dict& createPatchDict) const
{
  OFDictData::dict opdict;
  opdict["name"]=p_.name();
  opdict["constructFrom"]=p_.constructFrom();
  OFDictData::list pl;
  std::copy(p_.patches().begin(), p_.patches().end(), pl.begin());
  opdict["patches"]=pl;
  opdict["set"]=p_.set();

  OFDictData::dict opsubdict;
  opsubdict["type"]=p_.type();
  opdict["dictionary"]=opsubdict;

  createPatchDict.getList("patchInfo").push_back( opdict );
}
  
createPatchOperator* createPatchOperator::clone() const
{
  return new createPatchOperator(p_);
}

}

void createPatch(const OpenFOAMCase& ofc, 
		  const boost::filesystem::path& location, 
		  const boost::ptr_vector<createPatchOps::createPatchOperator>& ops,
		  bool overwrite
		)
{
  using namespace createPatchOps;
  
  OFDictData::dict createPatchDict;
  
  createPatchDict["matchTolerance"] = 1e-3;
  createPatchDict["pointSync"] = false;
  
  createPatchDict.addListIfNonexistent("patchInfo");  
  BOOST_FOREACH( const createPatchOperator& op, ops)
  {
    op.addIntoDictionary(createPatchDict);
  }
  
  // then write to file
  boost::filesystem::path dictpath = location / "system" / "createPatchDict";
  if (!exists(dictpath.parent_path())) 
  {
    boost::filesystem::create_directories(dictpath.parent_path());
  }
  
  {
    std::ofstream f(dictpath.c_str());
    writeOpenFOAMDict(f, createPatchDict, boost::filesystem::basename(dictpath));
  }

  std::vector<std::string> opts;
  if (overwrite) opts.push_back("-overwrite");
    
  ofc.executeCommand(location, "createPatch", opts);
}


}