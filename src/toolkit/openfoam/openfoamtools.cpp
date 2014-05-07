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
#include "openfoam/openfoamcaseelements.h"
#include "base/analysis.h"
#include "base/linearalgebra.h"

#include "boost/filesystem.hpp"
#include "boost/ptr_container/ptr_vector.hpp"
#include "boost/assign.hpp"
#include <boost/tokenizer.hpp>
#include "boost/regex.hpp"
#include "boost/foreach.hpp"
#include <boost/graph/graph_concepts.hpp>

#include <map>
#include <cmath>
#include <limits>

using namespace std;
using namespace arma;
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
  
  std::vector<std::string> opts;
  if ((ofc.OFversion()>=220) && (listTimeDirectories(location).size()==0)) opts.push_back("-constant");
  ofc.forkCommand(proc, location, "setSet", opts);
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

void copyPolyMesh(const boost::filesystem::path& from, const boost::filesystem::path& to, bool purify, bool ignoremissing)
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
      if (exists(source/gzname)) 
      {
	cout<<"Copying file "<<gzname<<endl;
	if (exists(target/gzname)) remove(target/gzname);
	copy_file(source/gzname, target/gzname);
      }
      else if (exists(source/fname))
      {
	cout<<"Copying file "<<fname<<endl;
	if (exists(target/fname)) remove(target/fname);
	copy_file(source/fname, target/fname);
      }
      else 
	if (!ignoremissing) throw insight::Exception("Essential mesh file "+fname+" not present in "+source.c_str());
    }
  }
  else
    throw insight::Exception("Not implemented!");
}

void create_symlink_force_overwrite(const path& source, const path& targ)
{
  if (is_symlink(targ))
    remove(targ);
  else
  {
    if (exists(targ))
      throw insight::Exception("Link target "+targ.string()+" exists and is not a symlink! Please remove manually first.");
  }
    
  create_symlink(source, targ);
}

void linkPolyMesh(const boost::filesystem::path& from, const boost::filesystem::path& to)
{
  path source(from/"polyMesh");
  path target(to/"polyMesh");
  if (!exists(target))
    create_directories(target);
  
  std::string cmd("ls "); cmd+=source.c_str();
  ::system(cmd.c_str());
  
  BOOST_FOREACH(const std::string& fname, 
		list_of<std::string>("boundary")("faces")("neighbour")("owner")("points")
		.convert_to_container<std::vector<std::string> >())
  {
    path gzname(fname.c_str()); gzname+=".gz";
    if (exists(source/gzname)) create_symlink_force_overwrite(source/gzname, target/gzname);
    else if (exists(source/fname)) create_symlink_force_overwrite(source/fname, target/fname);
    else throw insight::Exception("Essential mesh file "+fname+" not present in "+source.c_str());
  }
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

setFieldOperator::~setFieldOperator()
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
  setFieldDict.getList("regions").push_back( "fieldToCell" );
  setFieldDict.getList("regions").push_back( opdict );
  
}

setFieldOperator* fieldToCellOperator::clone() const
{
  return new fieldToCellOperator(p_);
}

boxToCellOperator::boxToCellOperator(Parameters const& p )
: setFieldOperator(p),
  p_(p)
{
}

void boxToCellOperator::addIntoDictionary(OFDictData::dict& setFieldDict) const
{
  OFDictData::dict opdict;
  opdict["box"]=OFDictData::to_OF(p_.min()) + OFDictData::to_OF(p_.max());

  OFDictData::list fve;
  BOOST_FOREACH(const FieldValueSpec& fvs, p_.fieldValues())
  {
    //std::ostringstream line;
    //line << fvs.get<0>() << " " << fvs.get<1>() ;
    fve.push_back( fvs );
  }
  opdict["fieldValues"]=fve;
  setFieldDict.getList("regions").push_back( "boxToCell" );
  setFieldDict.getList("regions").push_back( opdict );
}

setFieldOperator* boxToCellOperator::clone() const
{
  return new boxToCellOperator(p_);
}

}

void setFields(const OpenFOAMCase& ofc, const boost::filesystem::path& location, 
	       const std::vector<setFieldOps::FieldValueSpec>& defaultValues,
	       const boost::ptr_vector<setFieldOps::setFieldOperator>& ops)
{
  using namespace setFieldOps;
  
  OFDictData::dictFile setFieldsDict;
  
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
  boost::filesystem::path dictpath = location / "system" / "setFieldsDict";
  if (!exists(dictpath.parent_path())) 
  {
    boost::filesystem::create_directories(dictpath.parent_path());
  }
  
  {
    std::ofstream f(dictpath.c_str());
    writeOpenFOAMDict(f, setFieldsDict, boost::filesystem::basename(dictpath));
  }

  ofc.executeCommand(location, "setFields");
}

namespace createPatchOps
{

createPatchOperator::createPatchOperator(Parameters const& p )
: p_(p)
{
}

createPatchOperator::~createPatchOperator()
{
}

  
void createPatchOperator::addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& createPatchDict) const
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
  
  if (ofc.OFversion()<=160)
  {
    opdict["dictionary"]=opsubdict;
    createPatchDict.getList("patchInfo").push_back( opdict );
  }
  else
  {
    opdict["patchInfo"]=opsubdict;
    createPatchDict.getList("patches").push_back( opdict );
  }

}
  
createPatchOperator* createPatchOperator::clone() const
{
  return new createPatchOperator(p_);
}

createCyclicOperator::createCyclicOperator(Parameters const& p )
: p_(p)
{
}
  
void createCyclicOperator::addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& createPatchDict) const
{
  std::vector<std::string> suffixes;
  if (ofc.OFversion()>=210)
  {
    suffixes.push_back("_half0");
    suffixes.push_back("_half1");
  }
  else
    suffixes.push_back("");
  
  BOOST_FOREACH(const std::string& suf, suffixes)
  {
    OFDictData::dict opdict;
    opdict["name"]=p_.name()+suf;
    opdict["constructFrom"]=p_.constructFrom();
    OFDictData::list pl;
    if (suf=="_half0" || suf=="")
    {
      pl.resize(pl.size()+p_.patches().size());
      std::copy(p_.patches().begin(), p_.patches().end(), pl.begin());
      opdict["set"]=p_.set();
    }
    if (suf=="_half1" || suf=="")
    {
      int osize=pl.size();
      pl.resize(osize+p_.patches_half1().size());
      std::copy(p_.patches_half1().begin(), p_.patches_half1().end(), pl.begin()+osize);
      if (suf!="") opdict["set"]=p_.set_half1();
    }
    opdict["patches"]=pl;

    OFDictData::dict opsubdict;
    opsubdict["type"]="cyclic";
    if (suf=="_half0") opsubdict["neighbourPatch"]=p_.name()+"_half1";
    if (suf=="_half1") opsubdict["neighbourPatch"]=p_.name()+"_half0";
    
    if (ofc.OFversion()>=210)
    {
      opdict["patchInfo"]=opsubdict;
      createPatchDict.getList("patches").push_back( opdict );
    }
    else
    {
      opdict["dictionary"]=opsubdict;
      createPatchDict.getList("patchInfo").push_back( opdict );
    }
  }

}
  
createPatchOperator* createCyclicOperator::clone() const
{
  return new createCyclicOperator(p_);
}

}

void createPatch(const OpenFOAMCase& ofc, 
		  const boost::filesystem::path& location, 
		  const boost::ptr_vector<createPatchOps::createPatchOperator>& ops,
		  bool overwrite
		)
{
  using namespace createPatchOps;
  
  OFDictData::dictFile createPatchDict;
  
  createPatchDict["matchTolerance"] = 1e-3;
  createPatchDict["pointSync"] = false;
  
  if (ofc.OFversion()<=160)
    createPatchDict.addListIfNonexistent("patchInfo");  
  else
    createPatchDict.addListIfNonexistent("patches");  
  
  BOOST_FOREACH( const createPatchOperator& op, ops)
  {
    op.addIntoDictionary(ofc, createPatchDict);
  }
  
  // then write to file
  createPatchDict.write( location / "system" / "createPatchDict" );

  std::vector<std::string> opts;
  if (overwrite) opts.push_back("-overwrite");
    
  ofc.executeCommand(location, "createPatch", opts);
}

namespace sampleOps
{

set::set(Parameters const& p)
: p_(p)
{
}
  
set::~set()
{
}

uniformLine::uniformLine(Parameters const& p )
: set(p),
  p_(p)
{
}

void uniformLine::addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const
{
  OFDictData::list& l=sampleDict.addListIfNonexistent("sets");
  
  OFDictData::dict sd;
  sd["type"]="uniform";
  sd["axis"]="distance";
  sd["start"]=OFDictData::vector3(p_.start());
  sd["end"]=OFDictData::vector3(p_.end());
  sd["nPoints"]=p_.np();
  
  l.push_back(p_.name());
  l.push_back(sd);
}

set* uniformLine::clone() const
{
  return new uniformLine(p_);
}

arma::mat uniformLine::readSamples
(
  const OpenFOAMCase& ofc, const boost::filesystem::path& location, 
  const std::string& setName,
  ColumnDescription* coldescr
)
{
  arma::mat data;
  
  path fp;
  if (ofc.OFversion()<=160)
    fp=absolute(location)/"sets";
  else
    fp=absolute(location)/"postProcessing"/"sets";
  
  TimeDirectoryList tdl=listTimeDirectories(fp);
  
  BOOST_FOREACH(TimeDirectoryList::value_type& tde, tdl)
  {
    arma::mat m;
    std::vector<std::string> files;
    
    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( tde.second );
          itr != end_itr;
          ++itr )
    {      
      if ( is_regular_file(itr->status()) )
      {
        std::string fn=itr->path().filename().string();
	if (starts_with(fn, setName+"_")) files.push_back(fn);
      }
    }
      
    sort(files.begin(), files.end());
    
    for (int i=0; i<files.size(); i++)
    {
      std::string fn=files[i];
      path fp=tde.second/fn;

      erase_tail(fn, 3);
      std::vector<std::string> flnames;
      boost::split(flnames, fn, boost::is_any_of("_"));
      flnames.erase(flnames.begin());
      
      cout<<"Reading "<< fp <<endl;
      arma::mat res;	  
      res.load(fp.string(), arma::raw_ascii);
      cout << "Got size ("<< res.n_rows << "x" << res.n_cols << ")" << endl;
    
      int start_col=1;
      if (m.n_cols==0) m=res;
      else 
      {
	start_col=m.n_cols;
	m=arma::join_rows(m, res.cols(1, res.n_cols-1));
      }
      
      int ncmpt=(res.n_cols-1)/flnames.size();
      for (int j=0; j<flnames.size(); j++)
      {
	if (coldescr) 
	{
	  (*coldescr)[flnames[j]].ncmpt=ncmpt;
	  (*coldescr)[flnames[j]].col=start_col+ncmpt*j;
	}
      }
    }

    if (data.n_rows==0) 
      data=m;
    else 
      data=join_cols(data, m);
  }
  
  return data;
  
}




circumferentialAveragedUniformLine::circumferentialAveragedUniformLine(Parameters const& p )
: set(p),
  p_(p)
{
}

void circumferentialAveragedUniformLine::addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const
{
  OFDictData::list& l=sampleDict.addListIfNonexistent("sets");
  
  for (int i=0; i<p_.nc(); i++)
  {
    OFDictData::dict sd;
    sd["type"]="uniform";
    sd["axis"]="distance";
    sd["start"]=OFDictData::vector3(p_.start());
    sd["end"]=OFDictData::vector3(rotMatrix(i) * p_.end());
    sd["nPoints"]=p_.np();
    
    l.push_back(setname(i));
    l.push_back(sd);
  }
}

set* circumferentialAveragedUniformLine::clone() const
{
  return new circumferentialAveragedUniformLine(p_);
}

arma::mat circumferentialAveragedUniformLine::rotMatrix(int i) const
{
  return insight::rotMatrix( 2.*M_PI*double(i)/double(p_.nc()), p_.axis());
}

arma::mat circumferentialAveragedUniformLine::readSamples
(
  const OpenFOAMCase& ofc, const boost::filesystem::path& location, 
  ColumnDescription* coldescr
) const
{
  arma::mat data;
  ColumnDescription cd;
  for (int i=0; i< p_.nc(); i++)
  {
    arma::mat datai = uniformLine::readSamples(ofc, location, setname(i), &cd);
    arma::mat Ri=rotMatrix(i).t();
    
    BOOST_FOREACH(const ColumnDescription::value_type& fn, cd)
    {
      ColumnInfo ci=fn.second;
      cout<<fn.first<<": c="<<ci.col<<" ncmpt="<<ci.ncmpt<<endl;
      if (ci.ncmpt==1)
      {
	// scalar: no transform needed
      }
      else if (ci.ncmpt==3)
      {
	datai.cols(ci.col, ci.col+2) = (Ri * datai.cols(ci.col, ci.col+2).t()).t();
      }
      else if (ci.ncmpt==6)
      {
	// symmetric tensor
	int c0=ci.col;
	for (int j=0; j<datai.n_rows; j++)
	{
	  arma::mat t;
	  t << datai(j,c0)   << datai(j,c0+1) << datai(j,c0+2) << endr
	    << datai(j,c0+1) << datai(j,c0+3) << datai(j,c0+4) << endr
	    << datai(j,c0+2) << datai(j,c0+4) << datai(j,c0+5) << endr;
	   
	  t = Ri * t * Ri.t();
	  double symm=fabs(t(1,0)-t(0,1))+fabs(t(0,2)-t(2,0))+fabs(t(1,2)-t(2,1));
	  if (symm>1e-6) cout<<"Warning: unsymmetric tensor after rotation: "<<endl<<t<<endl;	  
	  
	  datai(j,c0)   = t(0,0);
	  datai(j,c0+1) = t(0,1);
	  datai(j,c0+2) = t(0,2);
	  datai(j,c0+3) = t(1,1);
	  datai(j,c0+4) = t(1,2);
	  datai(j,c0+5) = t(2,2);
	}
      }
      else
      {
	throw insight::Exception("circumferentialAveragedUniformLine::readSamples: encountered quantity with "
	    +lexical_cast<string>(ci.ncmpt)+" components. Don't know how to handle.");
      }
    }
    
    datai.save(p_.name()+"_circularinstance_i"+lexical_cast<string>(i)+".txt", arma::raw_ascii);
    
    if (i==0)
      data=datai;
    else
      data+=datai;
  }
  
  {
    std::ofstream f( (p_.name()+"_circularinstance_colheader.txt").c_str() );
    f<<"#";
    BOOST_FOREACH(const ColumnDescription::value_type& fn, cd)
    {
      ColumnInfo ci=fn.second;
      if (ci.ncmpt==0)
      {
	f<<" "+fn.first;
      }
      else
      {
	for (int c=0; c<ci.ncmpt; c++)
	  f<<" "+fn.first+"_"+lexical_cast<string>(c);
      }
      //cout<<fn.first<<": c="<<ci.col<<" ncmpt="<<ci.ncmpt<<endl;
    }
    f<<endl;
  }
  
  if (coldescr) *coldescr=cd;
  
  return data / double(p_.nc());
  
}



linearAveragedUniformLine::linearAveragedUniformLine(Parameters const& p )
: set(p),
  p_(p)
{
  x_=arma::linspace(0., norm(p_.end()-p_.start(),2), p_.np()); // valid for axis == distance!
}

void linearAveragedUniformLine::addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const
{
  OFDictData::list& l=sampleDict.addListIfNonexistent("sets");
  
  for (int i=0; i<p_.nd1(); i++)
    for (int j=0; j<p_.nd2(); j++)
    {
      arma::mat ofs = p_.dir1()*(double(i)/double(max(1,p_.nd1()-1))) + p_.dir2()*(double(j)/double(max(1,p_.nd2()-1)));
      OFDictData::dict sd;
      sd["type"]="uniform";
      sd["axis"]="distance";
      sd["start"]=OFDictData::vector3(p_.start()+ofs);
      sd["end"]=OFDictData::vector3(p_.end()+ofs);
      sd["nPoints"]=p_.np();
      
      l.push_back(setname(i, j));
      l.push_back(sd);
    }
    
}

set* linearAveragedUniformLine::clone() const
{
  return new linearAveragedUniformLine(p_);
}


arma::mat linearAveragedUniformLine::readSamples
(
  const OpenFOAMCase& ofc, const boost::filesystem::path& location, 
  ColumnDescription* coldescr
) const
{
  arma::mat data;
  
  ColumnDescription cd;
  for (int i=0; i<p_.nd1(); i++)
    for (int j=0; j<p_.nd2(); j++)
    {
      arma::mat datai = Interpolator(uniformLine::readSamples(ofc, location, setname(i, j), &cd))(x_);
      
      datai.save(p_.name()+"_linearinstance_i"+lexical_cast<string>(i)+"__j"+lexical_cast<string>(j)+".txt", arma::raw_ascii);
      
      if ((i==0) && (j==0))
	data=datai;
      else
	data+=datai;
    }
  
  if (coldescr) *coldescr=cd;
  
  return arma::mat(join_rows(x_, data / double(p_.nd1()*p_.nd2())));
  
}

}

void sample(const OpenFOAMCase& ofc, 
	    const boost::filesystem::path& location, 
	    const std::vector<std::string>& fields,
	    const boost::ptr_vector<sampleOps::set>& sets
	    )
{
  using namespace sampleOps;
  
  OFDictData::dictFile sampleDict;
  
  sampleDict["setFormat"] = "raw";
  sampleDict["surfaceFormat"] = "vtk";
  sampleDict["interpolationScheme"] = "cellPointFace";
  sampleDict["formatOptions"] = OFDictData::dict();
  
  OFDictData::list flds; flds.resize(fields.size());
  copy(fields.begin(), fields.end(), flds.begin());
  sampleDict["fields"] = flds;
  
  sampleDict["sets"] = OFDictData::list();
  sampleDict["surfaces"] = OFDictData::list();
    
  BOOST_FOREACH( const set& s, sets)
  {
    s.addIntoDictionary(ofc, sampleDict);
  }
  
  // then write to file
  sampleDict.write( location / "system" / "sampleDict" );

  std::vector<std::string> opts;
  opts.push_back("-latestTime");
  //if (overwrite) opts.push_back("-overwrite");
    
  ofc.executeCommand(location, "sample", opts);
  
}

void convertPatchPairToCyclic
(
  const OpenFOAMCase& ofc,
  const boost::filesystem::path& location, 
  const std::string& namePrefix
)
{
  using namespace createPatchOps;
  boost::ptr_vector<createPatchOperator> ops;
  
  ops.push_back(new createCyclicOperator( createCyclicOperator::Parameters()
   .set_name(namePrefix)
   .set_patches( list_of<string>(namePrefix+"_half0") )
   .set_patches_half1( list_of<string>(namePrefix+"_half1") )
  ) );
  
  createPatch(ofc, location, ops, true);
}

void mergeMeshes(const OpenFOAMCase& targetcase, const boost::filesystem::path& source, const boost::filesystem::path& target)
{
  targetcase.executeCommand
  (
    target, "mergeMeshes", 
    list_of<std::string>
    (".")
    (boost::filesystem::absolute(source).c_str()) 
  );
}


void mapFields
(
  const OpenFOAMCase& targetcase, 
  const boost::filesystem::path& source, 
  const boost::filesystem::path& target,
  bool parallelTarget
)
{
  path mfdPath=target / "system" / "mapFieldsDict";
  if (!exists(mfdPath))
  {
    OFDictData::dictFile mapFieldsDict;
    mapFieldsDict["patchMap"] = OFDictData::list();
    mapFieldsDict["cuttingPatches"] = OFDictData::list();
    mapFieldsDict.write( mfdPath );
  }
  else
  {
    cout<<"A mapFieldsDict is existing. It will be used."<<endl;
  }

  std::vector<string> args =
    list_of<std::string>
    (boost::filesystem::absolute(source).c_str())
    ("-sourceTime")("latestTime")
    ;
  if (parallelTarget) 
    args.push_back("-parallelTarget");
  
  if (targetcase.OFversion()>=230)
  {
    args.push_back("-mapMethod");
    args.push_back("mapNearest");
  }

  targetcase.executeCommand
  (
    target, "mapFields", args
  );
}


void resetMeshToLatestTimestep(const OpenFOAMCase& c, const boost::filesystem::path& location, bool ignoremissing)
{
  TimeDirectoryList times = listTimeDirectories(boost::filesystem::absolute(location));
  boost::filesystem::path lastTime = times.rbegin()->second;
  
  if (!ignoremissing) remove_all(location/"constant"/"polyMesh");
  copyPolyMesh(lastTime, location/"constant", true, ignoremissing);
  
  BOOST_FOREACH(const TimeDirectoryList::value_type& td, times)
  {
    remove_all(td.second);
  }
}

void runPotentialFoam
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  bool* stopFlagPtr,
  int np
)
{
  path fvSol(boost::filesystem::absolute(location)/"system"/"fvSolution");
  path fvSolBackup(fvSol); fvSolBackup.replace_extension(".potf");
  path fvSch(boost::filesystem::absolute(location)/"system"/"fvSchemes");
  path fvSchBackup(fvSch); fvSchBackup.replace_extension(".potf");
  
  if (exists(fvSol)) copy_file(fvSol, fvSolBackup, copy_option::overwrite_if_exists);
  if (exists(fvSch)) copy_file(fvSch, fvSchBackup, copy_option::overwrite_if_exists);
  
  OFDictData::dictFile fvSolution;
  OFDictData::dict& solvers=fvSolution.addSubDictIfNonexistent("solvers");
  solvers["p"]=stdSymmSolverSetup(1e-7, 0.01);
  fvSolution.addSubDictIfNonexistent("relaxationFactors");
  OFDictData::dict& potentialFlow=fvSolution.addSubDictIfNonexistent("potentialFlow");
  potentialFlow["nNonOrthogonalCorrectors"]=3;
  
  OFDictData::dictFile fvSchemes;
  fvSchemes.addSubDictIfNonexistent("ddtSchemes");
  fvSchemes.addSubDictIfNonexistent("gradSchemes");
  fvSchemes.addSubDictIfNonexistent("divSchemes");
  fvSchemes.addSubDictIfNonexistent("laplacianSchemes");
  fvSchemes.addSubDictIfNonexistent("interpolationSchemes");
  fvSchemes.addSubDictIfNonexistent("snGradSchemes");
  fvSchemes.addSubDictIfNonexistent("fluxRequired");
  
  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="steadyState";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  grad["default"]="Gauss linear";
  
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  div["default"]="Gauss upwind";

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear limited 0.66";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="limited 0.66";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["p"]="";
  fluxRequired["default"]="no";
  
  // then write to file
  {
    std::ofstream f(fvSol.c_str());
    writeOpenFOAMDict(f, fvSolution, boost::filesystem::basename(fvSol));
    f.close();
  }
  {
    std::ofstream f(fvSch.c_str());
    writeOpenFOAMDict(f, fvSchemes, boost::filesystem::basename(fvSch));
    f.close();
  }
  
  TextProgressDisplayer displayer;
  SolverOutputAnalyzer analyzer(displayer);
  cm.runSolver(location, analyzer, "potentialFoam", stopFlagPtr, np);

  if (exists(fvSol)) copy_file(fvSolBackup, fvSol, copy_option::overwrite_if_exists);
  if (exists(fvSch)) copy_file(fvSchBackup, fvSch, copy_option::overwrite_if_exists);
  
}

void runPvPython
(
  const OpenFOAMCase& ofc, 
  const boost::filesystem::path& location,
  const std::vector<std::string> pvpython_commands
)
{
  redi::opstream proc;  
  std::vector<string> args;
  args.push_back("--use-offscreen-rendering");
  std::string machine=""; // execute always on local machine
  //ofc.forkCommand(proc, location, "pvpython", args);
  
  path tempfile=absolute(unique_path("%%%%%%%%%.py"));
  {
    std::ofstream tf(tempfile.c_str());
    tf << "from Insight.Paraview import *" << endl;
    BOOST_FOREACH(const std::string& cmd, pvpython_commands)
    {
      tf << cmd;
    }
    tf.close();
  }
  args.push_back(tempfile.c_str());
  ofc.executeCommand(location, "pvbatch", args, NULL, 0, &machine);
  remove(tempfile);

}

arma::mat patchIntegrate(const OpenFOAMCase& cm, const boost::filesystem::path& location,
		    const std::string& fieldName, const std::string& patchName,
		   const std::vector<std::string>& addopts
			)
{
  std::vector<std::string> opts;
  opts.push_back(fieldName);
  opts.push_back(patchName);
  copy(addopts.begin(), addopts.end(), back_inserter(opts));
  
  std::vector<std::string> output;
  cm.executeCommand(location, "patchIntegrate", opts, &output);
  
  boost::regex re_time("^ *Time = (.+)$");
  boost::regex re_mag("^ *Integral of (.+) over area magnitude of patch (.+)\\[(.+)\\] = (.+)$");
  boost::match_results<std::string::const_iterator> what;
  double time=0;
  std::vector<double> data;
  BOOST_FOREACH(const std::string& line, output)
  {
    if (boost::regex_match(line, what, re_time))
    {
      //cout<< what[1]<<endl;
      time=lexical_cast<double>(what[1]);
      data.push_back(time);
    }
    if (boost::regex_match(line, what, re_mag))
    {
      //cout<<what[1]<<" : "<<what[4]<<endl;
      data.push_back(lexical_cast<double>(what[4]));
    }
  }
  
  return arma::mat(data.data(), 2, data.size()/2).t();
}

arma::mat readParaviewCSV(const boost::filesystem::path& filetemplate, std::map<std::string, int>* headers, int num)
{
  if (num<0)
    throw insight::Exception("readParaviewCSV: Reading and combining all files is not yet supported!");
  
  boost::filesystem::path file=filetemplate.parent_path() 
    / (filetemplate.filename().stem().string() + lexical_cast<string>(num) + filetemplate.filename().extension().string());
    
  cout << "Reading "<<file<<endl;
    
  std::ifstream f(file.c_str());
  
  std::vector<double> data;
  
  std::string headerline;
  getline(f, headerline);
  std::vector<std::string> colnames;
  boost::split(colnames, headerline, boost::is_any_of(","));
  for(size_t i=0; i<colnames.size(); i++)
  {
    (*headers)[colnames[i]]=i;
  }
  
  while(!f.eof())
  {
    std::string line;
    getline(f, line);
    if (f.fail()) break;
    
    std::vector<std::string> cols;
    boost::split(cols, line, boost::is_any_of(","));
    for(size_t i=0; i<cols.size(); i++)
    {
      data.push_back(lexical_cast<double>(cols[i]));
    }
  }
  
  return arma::mat(data.data(), colnames.size(), data.size()/colnames.size()).t();
}


int readDecomposeParDict(const boost::filesystem::path& ofcloc)
{
  OFDictData::dict decomposeParDict;
  std::ifstream cdf( (ofcloc/"system"/"decomposeParDict").c_str() );
  readOpenFOAMDict(cdf, decomposeParDict);
  //cout<<decomposeParDict<<endl;
  return decomposeParDict.getInt("numberOfSubdomains");
}

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
  
  MeshQualityInfo()
  {
    ncells=-1;
    nhex=-1;
    nprism=-1;
    ntet=-1;
    npoly=-1;
    nmeshregions=-1;
    bb_min=vec3(-DBL_MAX, -DBL_MAX, -DBL_MAX);
    bb_max=vec3(DBL_MAX, DBL_MAX, DBL_MAX);
    max_aspect_ratio=-1;
    min_faceA="";
    min_cellV="";
    max_nonorth=-1;
    avg_nonorth=-1;
    max_skewness=-1;
    n_severe_nonorth=0;
    n_neg_facepyr=0;
    n_severe_skew=0;
  }
};
  
void meshQualityReport(const OpenFOAMCase& cm, const boost::filesystem::path& location, 
		       ResultSetPtr results,
		       const std::vector<string>& addopts
		      )
{
  std::vector<std::string> opts;
  copy(addopts.begin(), addopts.end(), back_inserter(opts));
  
  std::vector<std::string> output;
  cm.executeCommand(location, "checkMesh", opts, &output);

  // Pattern
  enum Section {MeshStats, CellTypes, Topology, Geometry} ;
  boost::regex SectionIntroPattern[] = {
    boost::regex("^Mesh stats$"),
    boost::regex("^Overall number of cells of each type:$"),
    boost::regex("^Checking topology...$"),
    boost::regex("^Checking geometry...$")
  };
  Section curSection;
  
  boost::regex re_time("^ *Time = (.+)$");
  boost::match_results<std::string::const_iterator> what;
  std::string time="";
  

  
  typedef std::vector<MeshQualityInfo> MQInfoList;
  MQInfoList mqinfos;
  MeshQualityInfo curmq;
  BOOST_FOREACH(const std::string& line, output)
  {
    if (boost::regex_match(line, what, re_time))
    {
      if (curmq.time!="") 
      {
	mqinfos.push_back(curmq);
      }
      curmq.time=what[1];
    }
    for (int i=0; i<4; i++) 
      if (boost::regex_match(line, what, SectionIntroPattern[i])) curSection=static_cast<Section>(i);
      
//     try{
    switch (curSection)
    {
      case MeshStats:
      {
	if (boost::regex_match(line, what, boost::regex("^ *cells: *([0-9]+)$")))
	  curmq.ncells=lexical_cast<int>(what[1]);
	break;
      }
      case CellTypes:
      {
	if (boost::regex_match(line, what, boost::regex("^ *hexahedra: *([0-9]+)$")))
	  curmq.nhex=lexical_cast<int>(what[1]);
	if (boost::regex_match(line, what, boost::regex("^ *prisms: *([0-9]+)$")))
	  curmq.nprism=lexical_cast<int>(what[1]);
	if (boost::regex_match(line, what, boost::regex("^ *tetrahedra: *([0-9]+)$")))
	  curmq.ntet=lexical_cast<int>(what[1]);
	if (boost::regex_match(line, what, boost::regex("^ *polyhedra: *([0-9]+)$")))
	  curmq.npoly=lexical_cast<int>(what[1]);
	break;
      }
      case Topology:
      {
	if (boost::regex_match(line, what, boost::regex("^ *Number of regions: *([^ ]+) .*$")))
	  curmq.nmeshregions=lexical_cast<int>(what[1]);
	break;
      }
      case Geometry:
      {
	if (boost::regex_match(line, what, boost::regex("^ *Overall domain bounding box \\(([^ ]+) ([^ ]+) ([^ ]+)\\) \\(([^ ]+) ([^ ]+) ([^ ]+)\\)$")))
	{
	  curmq.bb_min=vec3( lexical_cast<double>(what[1]), lexical_cast<double>(what[2]), lexical_cast<double>(what[3]) );
	  curmq.bb_max=vec3( lexical_cast<double>(what[4]), lexical_cast<double>(what[5]), lexical_cast<double>(what[6]) );
	}
	if (boost::regex_match(line, what, boost::regex("^ *Max aspect ratio = ([^ ]+) .*$")))
	{
	  curmq.max_aspect_ratio=lexical_cast<double>(what[1]);
	}
	if (boost::regex_match(line, what, boost::regex("^ *Minimum face area = *([^ ]+)\\. Maximum face area = *([^ ]+)\\..*$")))
	{
	  cout<<what[1]<<endl;
	  curmq.min_faceA=std::string(what[1]); // is a very small value, keep as string
	  cout<<curmq.min_faceA<<endl;
	  //sscanf(string(what[1]).data(), "%g", &curmq.min_faceA);
	}
	if (boost::regex_match(line, what, boost::regex("^ *Min volume = *([^ ]+)\\. Max volume.*$")))
	{
	  cout<<what[1]<<endl;
	  curmq.min_cellV=std::string(what[1]); // is a very small value, keep as string
	  cout<<curmq.min_cellV<<endl;
	  //sscanf(string(what[1]).data(), "%g", &curmq.min_cellV);
	}
	if (boost::regex_match(line, what, boost::regex("^ *Mesh non-orthogonality Max: ([^ ]+) average: ([^ ]+)$")))
	{
	  curmq.max_nonorth=lexical_cast<double>(what[1]);
	  curmq.avg_nonorth=lexical_cast<double>(what[2]);
	}
	if (boost::regex_match(line, what, boost::regex("^.*Number of severely non-orthogonal \\(> ([^ ]+) degrees\\) faces: ([^ ]+)\\..*$")))
	{
	  curmq.n_severe_nonorth=lexical_cast<double>(what[2]);
	}
	if (boost::regex_match(line, what, boost::regex("^.*Max skewness = ([^ ]+), ([^ ]+) highly skew faces.*$")))
	{
	  curmq.n_severe_skew=lexical_cast<double>(what[2]);
	  curmq.max_skewness=lexical_cast<double>(what[1]);
	}
	if (boost::regex_match(line, what, boost::regex("^.*Max skewness = ([^ ]+) OK.*$")))
	{
	  curmq.n_severe_nonorth=lexical_cast<double>(what[1]);
	}
	if (boost::regex_match(line, what, boost::regex("^.*Error in face pyramids: ([^ ]+) faces are incorrectly oriented.*$")))
	{
	  curmq.n_neg_facepyr=lexical_cast<double>(what[1]);
	}
	break;
      }
    }
//     }
//     catch(boost::bad_lexical_cast& e) {
//      cout<<"Lexical_cast: "<<e.what()<<endl;
//     }
  }
  if (curmq.time!="") mqinfos.push_back(curmq);
  
  BOOST_FOREACH(const MeshQualityInfo& mq, mqinfos)
  {
    results->insert
    (
     "Mesh quality at time "+mq.time,
     std::auto_ptr<AttributeTableResult>
     (
       new AttributeTableResult
       (
	 list_of<string>
	  ("Number of cells")
	  ("thereof hexahedra")
	  ("prisms")
	  ("tetrahedra")
	  ("polyhedra")
	  
	  ("Number of mesh regions")
	  
	  ("Domain extent (X)")
	  ("Domain extent (Y)")
	  ("Domain extent (Z)")
	  
	  ("Max. aspect ratio")
	  ("Min. face area")
	  ("Min. cell volume")
	  
	  ("Max. non-orthogonality")
	  ("Avg. non-orthogonality")
	  ("Max. skewness")
	  
	  ("No. of severely non-orthogonal faces")
	  ("No. of negative face pyramids")
	  ("No. of severely skew faces"),

	 list_of<AttributeTableResult::AttributeValue>
	  (mq.ncells)(mq.nhex)(mq.nprism)(mq.ntet)(mq.npoly)
	  (mq.nmeshregions)
	  (mq.bb_max(0)-mq.bb_min(0))(mq.bb_max(1)-mq.bb_min(1))(mq.bb_max(2)-mq.bb_min(2))
	  (mq.max_aspect_ratio)(mq.min_faceA)(mq.min_cellV)
	  (mq.max_nonorth)(mq.avg_nonorth)(mq.max_skewness)
	  (mq.n_severe_nonorth)(mq.n_neg_facepyr)(mq.n_severe_skew),
	"Mesh Quality", "", ""
	)
     )
    );
  }
}

arma::mat viscousForceProfile
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const arma::mat& axis, int n,
  const std::vector<std::string>& addopts
)
{
  std::vector<std::string> opts;
  opts.push_back(OFDictData::to_OF(axis));
  opts.push_back("-n");
  opts.push_back(lexical_cast<string>(n));
  copy(addopts.begin(), addopts.end(), back_inserter(opts));
  
  std::vector<std::string> output;
  cm.executeCommand(location, "viscousForceProfile", opts, &output);
  
  path pref=location/"postProcessing"/"viscousForceProfile";
  TimeDirectoryList tdl=listTimeDirectories(pref);
  path lastTimeDir=tdl.rbegin()->second;
  arma::mat vfm;
  vfm.load( ( lastTimeDir/"viscousForceMean.dat").c_str(), arma::raw_ascii);
  arma::mat vf;
  vf.load( (lastTimeDir/"viscousForce.dat").c_str(), arma::raw_ascii);
  
  return arma::mat(join_rows(vfm, vf.col(1)));
}

}
