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
#include "openfoam/openfoamtools.h"
#include "base/exception.h"
#include <base/analysis.h>
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/openfoamdict.h"


#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace insight::OFDictData;


namespace insight
{
  
OFDictData::dictFile& OFdicts::addDictionaryIfNonexistent(const std::string& key)
{
  OFdicts::iterator i=find(key);
  if (i==end())
  {
    (*this)[key]=OFDictData::dictFile();
  }
  return (*this)[key];
}

OFDictData::dictFile& OFdicts::addFieldIfNonexistent(const std::string& key, const FieldInfo& fi)
{
  OFDictData::dictFile& d=addDictionaryIfNonexistent(key);
  std::string className;
  if (boost::fusion::get<3>(fi) == volField )
    className="vol";
  else if (boost::fusion::get<3>(fi) == pointField )
    className="point";
  else if (boost::fusion::get<3>(fi) == tetField )
    className="tetPoint";
  else
    throw insight::Exception("Don't know the geotype of field "+lexical_cast<std::string>(boost::fusion::get<3>(fi)));
    
  if (boost::fusion::get<0>(fi)==scalarField)
    className+="ScalarField";
  else if (boost::fusion::get<0>(fi)==vectorField)
    className+="VectorField";
  else if (boost::fusion::get<0>(fi)==symmTensorField)
    className+="SymmTensorField";
  else
    throw insight::Exception("Don't know the class name of field type "+lexical_cast<std::string>(boost::fusion::get<0>(fi)));
  
  d.className=className;

  return d;
}

OFDictData::dictFile& OFdicts::lookupDict(const std::string& key)
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

OFEs OFEs::list;

const OFEnvironment& OFEs::get(const std::string& name)
{
  const_iterator it=list.find(name);
  if (it==list.end())
    throw insight::Exception
    (
      "OFEs::get(): Requested OpenFOAM environment "+name+" is undefined.\n"
      "(Check environment variable INSIGHT_OFES)"
    );
  
  return *(it->second);
}

OFEs::OFEs()
{
  const char *envvar=getenv("INSIGHT_OFES");
  if (!envvar)
  {
    cout<<"Warning: No OpenFOAM installations defined! (environment variable INSIGHT_OFES)"<<endl;
    return;
  }
  std::string cfgvar(envvar);
  std::vector<std::string> ofestrs;
  boost::split(ofestrs, cfgvar, boost::is_any_of(":"));
  BOOST_FOREACH(const std::string& ofe, ofestrs)
  {
    std::vector<std::string> strs;
    boost::split(strs, ofe, boost::is_any_of("@#"));
    //cout << "adding " << strs[0] << ": "<<strs[1] << " (version "<<strs[2]<<")"<<endl;
    (*this).insert(strs[0], new OFEnvironment(lexical_cast<int>(strs[2]), strs[1]));
  }
}

OFEs::~OFEs()
{
}

/*
int OFEnvironment::executeCommand(const std::vector<std::string>& args) const
{
}
*/


int OpenFOAMCaseElement::OFversion() const 
{
  return OFcase().OFversion();
}

OpenFOAMCaseElement::OpenFOAMCaseElement(OpenFOAMCase& c, const std::string& name)
: CaseElement(c, name)
{
}

bool OpenFOAMCaseElement::providesBCsForPatch(const std::string& patchName) const
{
  return false;
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


const OFDictData::dimensionSet dimPressure = OFDictData::dimension(1, -1, -2, 0, 0, 0, 0);
const OFDictData::dimensionSet dimKinPressure = OFDictData::dimension(0, 2, -2, 0, 0, 0, 0);
const OFDictData::dimensionSet dimKinEnergy = OFDictData::dimension(0, 2, -2, 0, 0, 0, 0);
const OFDictData::dimensionSet dimVelocity = OFDictData::dimension(0, 1, -1, 0, 0, 0, 0);
const OFDictData::dimensionSet dimLength = OFDictData::dimension(0, 1, 0, 0, 0, 0, 0);
const OFDictData::dimensionSet dimDensity = OFDictData::dimension(1, -3, 0, 0, 0, 0, 0);
const OFDictData::dimensionSet dimless = OFDictData::dimension(0, 0, 0, 0, 0, 0, 0);
const OFDictData::dimensionSet dimKinViscosity = OFDictData::dimension(0, 2, -1, 0, 0, 0, 0);

boost::shared_ptr<OFdicts> OpenFOAMCase::createDictionaries() const
{
  boost::shared_ptr<OFdicts> dictionaries(new OFdicts);

  // create field dictionaries first
  BOOST_FOREACH( const FieldList::value_type& i, fields_)
  {
    OFDictData::dictFile& field = dictionaries->addFieldIfNonexistent("0/"+i.first, i.second);
    
    const dimensionSet& dimset=boost::fusion::get<1>(i.second);
    std::ostringstream dimss; 
    //dimss << dimset;
    dimss <<"[ "; BOOST_FOREACH(int c, dimset) dimss <<c<<" "; dimss<<"]";
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
	 cout<<e<<endl;
	 if (e)
	 {
	   cout<<e->name()<<endl;
	   e->addIntoDictionaries(*dictionaries);
	 }
       }  

  return dictionaries;
}


void OpenFOAMCase::createOnDisk(const boost::filesystem::path& location)
{
  boost::shared_ptr<OFdicts> dictionaries=createDictionaries();
  createOnDisk(location, dictionaries);
}


void OpenFOAMCase::createOnDisk(const boost::filesystem::path& location, boost::shared_ptr<OFdicts> dictionaries)
{
  boost::filesystem::path basepath(location);

  for (OFdicts::const_iterator i=dictionaries->begin();
      i!=dictionaries->end(); i++)
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

bool OpenFOAMCase::meshPresentOnDisk( const boost::filesystem::path& location ) const
{
  path meshPath = location/"constant"/"polyMesh";
  return
    exists(meshPath) &&
    ( exists(meshPath/"points") || exists(meshPath/"points.gz") ) &&
    ( exists(meshPath/"faces") || exists(meshPath/"faces.gz") ) &&
    ( exists(meshPath/"neighbour") || exists(meshPath/"neighbour.gz") ) &&
    ( exists(meshPath/"owner") || exists(meshPath/"owner.gz") ) &&
    exists(meshPath/"boundary");
}

bool OpenFOAMCase::outputTimesPresentOnDisk( const boost::filesystem::path& location ) const
{
  TimeDirectoryList timedirs=listTimeDirectories(location);
  return (timedirs.size()>1);
}

void OpenFOAMCase::removeProcessorDirectories( const boost::filesystem::path& location ) const
{
  boost::regex re_pdn("^processor([0-9]+)$");
  std::vector<path> list;
  if ( exists( location ) ) 
  {
    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( location );
          itr != end_itr;
          ++itr )
    {
      if ( is_directory(itr->status()) )
      {
	boost::match_results<std::string::const_iterator> what;
        std::string fn=itr->path().filename().string();
	if (boost::regex_match(fn, what, re_pdn))
	  remove_all(itr->path());
      }
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

/*
void OpenFOAMCase::addRemainingBCs(OFDictData::dict& boundaryDict, const std::string& className)
{
  typedef std::set<std::string> StringSet;
  StringSet unhandledPatches;
  BOOST_FOREACH(const OFDictData::dict::value_type& bde, boundaryDict)
  {
    unhandledPatches.insert(bde.first);
  }
  
  for (boost::ptr_vector<CaseElement>::const_iterator i=elements_.begin();
       i!=elements_.end(); i++)
       {
	 const BoundaryCondition *e= dynamic_cast<const BoundaryCondition*>(&(*i));
	 if (e)
	 {
	   StringSet::iterator i=unhandledPatches.find(e->patchName());
	   if (i!=unhandledPatches.end()) unhandledPatches.erase(i);
	 }
       }
       
  for (StringSet::const_iterator i=unhandledPatches.begin(); i!=unhandledPatches.end(); i++)
  {
    insert(new SimpleBC(*this, *i, boundaryDict, className));  
  }
}
*/

std::string OpenFOAMCase::cmdString
(
  const boost::filesystem::path& location, 
  const std::string& cmd,
  std::vector<std::string> argv
)
const
{
  std::string shellcmd;
  shellcmd = 
    "source "+env_.bashrc().string()+";"
    "export PATH=$PATH:$INSIGHT_BINDIR/$WM_PROJECT-$WM_PROJECT_VERSION;"
    "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$INSIGHT_LIBDIR/$WM_PROJECT-$WM_PROJECT_VERSION;"
    "cd \""+boost::filesystem::absolute(location).string()+"\";"
    + cmd;
  BOOST_FOREACH(std::string& arg, argv)
  {
    shellcmd+=" \""+arg+"\"";
  }
  cout<<shellcmd<<endl;
  return shellcmd;
}

void OpenFOAMCase::executeCommand
(
  const boost::filesystem::path& location, 
  const std::string& cmd,
  std::vector<std::string> argv,
  std::vector<std::string>* output,
  int np,
  std::string *ovr_machine
) const
{
  string execmd=cmd;
  if (np>1)
  {
    execmd="mpirun -np "+lexical_cast<string>(np)+" "+cmd;
    argv.push_back("-parallel");
  }
  
  env_.executeCommand( cmdString(location, execmd, argv), std::vector<std::string>(), output, ovr_machine );
}

void OpenFOAMCase::runSolver
(
  const boost::filesystem::path& location, 
  SolverOutputAnalyzer& analyzer,
  std::string solverName,
  bool *stopFlag,
  int np,
  const std::vector<std::string>& addopts
) const
{
  if (stopFlag) *stopFlag=false;
  
  redi::ipstream p_in;
  
  string cmd=solverName;
  std::vector<std::string> argv;
  if (np>1)
  {
    cmd="mpirun -np "+lexical_cast<string>(np)+" "+cmd;
    argv.push_back("-parallel");
  }
  std::copy(addopts.begin(), addopts.end(), back_inserter(argv));

  //env_.forkCommand( p_in, "bash", boost::assign::list_of<std::string>("-c")(shellcmd) );
  env_.forkCommand( p_in, cmdString(location, cmd, argv) );

  std::string line;
  while (std::getline(p_in, line))
  {
    cout<<">> "<<line<<endl;
    analyzer.update(line);
    
    boost::this_thread::interruption_point();
    if (stopFlag) { if (*stopFlag) break; }
  }
  p_in.close();

  if (p_in.rdbuf()->status()!=0)
    throw insight::Exception("OpenFOAMCase::runSolver(): solver execution failed with nonzero exeit code!");
}

std::set<std::string> OpenFOAMCase::getUnhandledPatches(OFDictData::dict& boundaryDict) const
{
  typedef std::set<std::string> StringSet;
  StringSet unhandledPatches;
  BOOST_FOREACH(const OFDictData::dict::value_type& bde, boundaryDict)
  {
    unhandledPatches.insert(bde.first);
  }
  
  for (boost::ptr_vector<CaseElement>::const_iterator i=elements_.begin();
      i!=elements_.end(); i++)
      {
	const OpenFOAMCaseElement* e= dynamic_cast<const OpenFOAMCaseElement*>(&(*i));
	BOOST_FOREACH(const OFDictData::dict::value_type& bde, boundaryDict)
	{
	  std::string pn(bde.first);
	  
	  if (e->providesBCsForPatch(pn))
	  {
	    StringSet::iterator i=unhandledPatches.find(pn);
	    if (i!=unhandledPatches.end()) unhandledPatches.erase(i);
	  }
	}
	
	/*
	const BoundaryCondition *e= dynamic_cast<const BoundaryCondition*>(&(*i));
	if (e)
	{
	  StringSet::iterator i=unhandledPatches.find(e->patchName());
	  if (i!=unhandledPatches.end()) unhandledPatches.erase(i);
	}
	*/
      }
      
   return unhandledPatches;
}
}