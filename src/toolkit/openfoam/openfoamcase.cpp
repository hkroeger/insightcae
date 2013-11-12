/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  hannes <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "boost/filesystem.hpp"
#include "boost/assign.hpp"
#include "openfoam/openfoamcase.h"
#include "base/exception.h"
#include <base/analysis.h>

#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace insight::OFDictData;


namespace insight
{
  
OFDictData::dict& OFdicts::addDictionaryIfNonexistent(const std::string& key)
{
  OFdicts::iterator i=find(key);
  if (i==end())
  {
    (*this)[key]=OFDictData::dict();
  }
  return (*this)[key];
}

OFDictData::dict& OFdicts::lookupDict(const std::string& key)
{
  OFdicts::iterator i=find(key);
  if (i==end())
  {
    throw Exception("Dictionary "+key+" not found!");
  }
  return *(i->second);
}

  
OFEnvironment::OFEnvironment(int version, const boost::filesystem::path& bashrc)
: version_(version),
  bashrc_(bashrc)
{
}

int OFEnvironment::version() const
{
  return version_;
}

const boost::filesystem::path& OFEnvironment::bashrc() const
{
  return bashrc_;
}

/*
int OFEnvironment::executeCommand(const std::vector<std::string>& args) const
{
}
*/

const OpenFOAMCase& OpenFOAMCaseElement::OFcase() const
{ 
  return *static_cast<OpenFOAMCase*>(&case_); 
}

int OpenFOAMCaseElement::OFversion() const 
{
  return OFcase().OFversion();
}

OpenFOAMCaseElement::OpenFOAMCaseElement(OpenFOAMCase& c, const std::string& name)
: CaseElement(c, name)
{
}

SolverOutputAnalyzer::SolverOutputAnalyzer(ProgressDisplayer& pdisp)
: pdisp_(pdisp),
  curTime_(nan("NAN"))
{
}

void SolverOutputAnalyzer::update(const string& line)
{
  boost::regex time_pattern("^Time = (.+)$");
  boost::regex solver_pattern("^(.+): +Solving for (.+), Initial residual = (.+), Final residual = (.+), No Iterations (.+)$");
  boost::regex cont_pattern("^time step continuity errors : sum local = (.+), global = (.+), cumulative = (.+)$");

  boost::smatch match;
  if ( boost::regex_search( line, match, time_pattern, boost::match_default ) )
  {
    if (curTime_ == curTime_)
    {
      pdisp_.update( ProgressState(curTime_, curProgVars_));
    }
    curTime_=lexical_cast<double>(match[1]);
  } 
  else if ( boost::regex_search( line, match, solver_pattern, boost::match_default ) )
  {
    curProgVars_[match[2]] = lexical_cast<double>(match[3]);
  }
  else if ( boost::regex_search( line, match, cont_pattern, boost::match_default ) )
  {
    /*
    curProgVars_["local cont. err"] = lexical_cast<double>(match[1]);
    curProgVars_["global cont. err"] = lexical_cast<double>(match[2]);
    curProgVars_["cumul cont. err"] = lexical_cast<double>(match[3]);
    */
  }
}


const OFDictData::dimensionSet dimKinPressure = OFDictData::dimension(0, 2, -2, 0, 0, 0, 0);
const OFDictData::dimensionSet dimKinEnergy = OFDictData::dimension(0, 2, -2, 0, 0, 0, 0);
const OFDictData::dimensionSet dimVelocity = OFDictData::dimension(0, 1, -1, 0, 0, 0, 0);

void OpenFOAMCase::createOnDisk(const boost::filesystem::path& location)
{
  boost::filesystem::path basepath(location);

  OFdicts dictionaries_;
  
  BOOST_FOREACH( const FieldList::value_type& i, fields_)
  {
    OFDictData::dict& field = dictionaries_.addDictionaryIfNonexistent("0/"+i.first);
    std::ostringstream dimss; dimss << boost::fusion::get<1>(i.second);
    field["dimensions"] = OFDictData::data( dimss.str() );
    std::string vstr="";
    const FieldValue& val = boost::fusion::get<2>(i.second);
    BOOST_FOREACH( const double& v, val)
    {
      vstr+=" "+lexical_cast<std::string>(v);
    }
    if (val.size()>1) vstr  = " ("+vstr+" )";
    field["internalField"] = OFDictData::data( "uniform"+vstr );
    field["boundaryField"] = OFDictData::dict();
  }
  
  for (boost::ptr_vector<CaseElement>::const_iterator i=elements_.begin();
       i!=elements_.end(); i++)
       {
	 const OpenFOAMCaseElement *e= dynamic_cast<const OpenFOAMCaseElement*>(&(*i));
	 if (e)
	 {
	   e->addIntoDictionaries(dictionaries_);
	 }
       }
       
  for (OFdicts::const_iterator i=dictionaries_.begin();
      i!=dictionaries_.end(); i++)
      {
	/*
	// write to console for debug
	cout << endl
	      << i->first << endl 
	      << "=================" << endl;
	writeOpenFOAMDict(cout, *i->second, boost::filesystem::basename(i->first));
	*/
	// then write to file
	boost::filesystem::path dictpath = basepath / i->first;
	if (!exists(dictpath.parent_path())) 
	{
	  boost::filesystem::create_directories(dictpath.parent_path());
	}
	
	{
	  std::ofstream f(dictpath.c_str());
	  writeOpenFOAMDict(f, *i->second, boost::filesystem::basename(i->first));
	}
      }
}

OpenFOAMCase::OpenFOAMCase(const OFEnvironment& env)
: Case(),
  env_(env)
{
}

OpenFOAMCase::OpenFOAMCase(const OpenFOAMCase& other)
: Case(other),
  env_(other.env_)
{
}

OpenFOAMCase::~OpenFOAMCase()
{
}

void OpenFOAMCase::addField(const std::string& name, const FieldInfo& field)
{
  fields_[name]=field;
}

void OpenFOAMCase::parseBoundaryDict(const boost::filesystem::path& location, OFDictData::dict& boundaryDict)
{
  boost::filesystem::path basepath(location);
  boost::filesystem::path dictpath = basepath / "constant" / "polyMesh" / "boundary";
  std::ifstream f(dictpath.c_str());
  readOpenFOAMBoundaryDict(f, boundaryDict);
}

void OpenFOAMCase::addRemainingBCs(OFDictData::dict& boundaryDict)
{
}

int OpenFOAMCase::executeCommand
(
  const boost::filesystem::path& location, 
  const std::string& cmd,
  std::vector<std::string> argv,
  std::vector<std::string>* output
) const
{
  std::string shellcmd;
  shellcmd = "source "+env_.bashrc().string()+";cd \""+location.string()+"\";"+cmd;
  BOOST_FOREACH(std::string& arg, argv)
  {
    shellcmd+=" \""+arg+"\"";
  }
  return env_.executeCommand( "bash", boost::assign::list_of<std::string>("-c")(shellcmd), output );
}

int OpenFOAMCase::runSolver
(
  const boost::filesystem::path& location, 
  SolverOutputAnalyzer& analyzer,
  std::string solverName,
  bool *stopFlag
)
{
  if (stopFlag) *stopFlag=false;
  
  std::string shellcmd;
  shellcmd = "source "+env_.bashrc().string()+";cd \""+location.string()+"\";"+solverName;

  redi::ipstream p_in;
  env_.forkCommand( p_in, "bash", boost::assign::list_of<std::string>("-c")(shellcmd) );

  std::string line;
  while (std::getline(p_in, line))
  {
    cout<<">> "<<line<<endl;
    analyzer.update(line);
    
    if (stopFlag) { if (*stopFlag) break; }
  }
  p_in.close();

  return p_in.rdbuf()->status();
}


}