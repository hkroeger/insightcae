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

#include "base/boost_include.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"
#include "base/exception.h"
#include <base/analysis.h>
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/openfoamdict.h"


using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace insight::OFDictData;


namespace insight
{
  
    
    
    
OFDictData::dictFile& OFdicts::addDictionaryIfNonexistent ( const std::string& key )
{
    OFdicts::iterator i=find ( key );
    if ( i==end() ) {
        ( *this ) [key]=OFDictData::dictFile();
    }
    return ( *this ) [key];
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

std::vector<std::string> OFEs::all()
{
    std::vector<std::string> entries;
    BOOST_FOREACH(const value_type& vr, OFEs::list)
    {
//         std::cout<<vr.first<<std::endl;
        entries.push_back(vr.first);
    }
    return entries;
}

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

std::string OFEs::detectCurrentOFE()
{
  const char *envvar=getenv("WM_PROJECT_DIR");
  if (!envvar)
  {
    return std::string();
  }
  BOOST_FOREACH(const OFEs::value_type& ofe, list)
  {
    std::vector<std::string> output;
    OpenFOAMCase(*ofe.second).executeCommand(".", "echo $WM_PROJECT_DIR", std::vector<std::string>(), &output);
    if (output.size()!=1) return std::string();
    boost::filesystem::path ofedir = boost::filesystem::absolute(boost::filesystem::path(*output.begin()));
    
    std::cout<<
      ofedir
      <<" =?= "<<
      boost::filesystem::absolute( boost::filesystem::path(envvar))<<std::endl;
    if (
     ofedir
      ==
      boost::filesystem::absolute( boost::filesystem::path(envvar))
    )
      return ofe.first;
  }
  return std::string();
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
    if (strs.size()==3)
    {
        try
        {
            (*this).insert(strs[0], new OFEnvironment(lexical_cast<int>(strs[2]), strs[1]));
        }
        catch (...)
        {
            insight::Warning("Error while parsing INSIGHT_OFES environment variable: OFE specification is invalid: \""+ofe+"\"");
        }
    }
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

defineType(OpenFOAMCaseElement);
defineFactoryTable(OpenFOAMCaseElement, LIST ( OpenFOAMCase& c, const ParameterSet& ps ), LIST ( c, ps ));
defineStaticFunctionTable(OpenFOAMCaseElement, defaultParameters, ParameterSet);
defineStaticFunctionTable(OpenFOAMCaseElement, category, std::string);


int OpenFOAMCaseElement::OFversion() const 
{
  return OFcase().OFversion();
}

void OpenFOAMCaseElement::modifyMeshOnDisk(const OpenFOAMCase& cm, const boost::filesystem::path& location) const
{
}

void OpenFOAMCaseElement::modifyCaseOnDisk(const OpenFOAMCase& cm, const boost::filesystem::path& location) const
{
}


void OpenFOAMCaseElement::addFields( OpenFOAMCase& c ) const
{
}


OpenFOAMCaseElement::OpenFOAMCaseElement(OpenFOAMCase& c, const std::string& name)
: CaseElement(c, name)
{
}

bool OpenFOAMCaseElement::providesBCsForPatch(const std::string& patchName) const
{
  return false;
}




defineType(BoundaryCondition);
defineFactoryTable
(
    BoundaryCondition, 
    LIST 
    (  
        OpenFOAMCase& c, 
        const std::string& patchName, 
        const OFDictData::dict& boundaryDict, 
        const ParameterSet& ps
    ),
    LIST ( c, patchName, boundaryDict, ps ) 
);
defineStaticFunctionTable(BoundaryCondition, defaultParameters, ParameterSet);


BoundaryCondition::BoundaryCondition(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict)
: OpenFOAMCaseElement(c, patchName+"BC"),
  patchName_(patchName),
  BCtype_(boundaryDict.subDict(patchName).getString("type")),
  nFaces_(boundaryDict.subDict(patchName).getInt("nFaces")),
  startFace_(boundaryDict.subDict(patchName).getInt("startFace"))  
{
}

void BoundaryCondition::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["type"]=BCtype_;
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
}

void BoundaryCondition::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dictFile& fieldDict=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second);
    OFDictData::dict& boundaryField=fieldDict.addSubDictIfNonexistent("boundaryField");
    OFDictData::dict& BC=boundaryField.addSubDictIfNonexistent(patchName_);
  }
}

void BoundaryCondition::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict.addListIfNonexistent("libs").insertNoDuplicate( OFDictData::data("\"libextendedFixedValueBC.so\"") );

  addIntoFieldDictionaries(dictionaries);
  
  OFDictData::dict bndsubd;
  addOptionsToBoundaryDict(bndsubd);
  insertIntoBoundaryDict(dictionaries, patchName_, bndsubd);
}

void BoundaryCondition::insertIntoBoundaryDict
(
  OFdicts& dictionaries, 
  const string& patchName,
  const OFDictData::dict& bndsubd
)
{
  // contents is created as list of string / subdict pairs
  // patches have to appear ordered by "startFace"!
  OFDictData::dict& boundaryDict=dictionaries.addDictionaryIfNonexistent("constant/polyMesh/boundary");
  if (boundaryDict.size()==0)
    boundaryDict.addListIfNonexistent("");
  
  OFDictData::list& bl=
    *boost::get<OFDictData::list>( &boundaryDict.begin()->second );
  
  //std::cout<<"Configuring "<<patchName_<<std::endl;
  // search, if patchname is already present; replace, if yes
  for(OFDictData::list::iterator i=bl.begin(); i!=bl.end(); i++)
  {
    if (std::string *name = boost::get<std::string>(&(*i)))
    {
      //cout<<"found "<<*name<<endl;
      if ( *name == patchName )
      {
	i++;
	*i=bndsubd;
	return;
      }
    }
  }
  
  // not found, insert (at the right location)
  OFDictData::list::iterator j = bl.end();
  for(OFDictData::list::iterator i=bl.begin(); i!=bl.end(); i++)
  {
    if (OFDictData::dict *d = boost::get<OFDictData::dict>(&(*i)))
    {
      if (d->getInt("startFace") > bndsubd.getInt("startFace") ) 
      {
	//std::cout << "Inserting before " << *boost::get<std::string>(&(*(i-1))) << std::endl;
	j=i-1;
	break;
      }
      // patch with 0 faces has to be inserted before the face with the same start address but nonzero size
      if ( (d->getInt("startFace") == bndsubd.getInt("startFace") ) && (bndsubd.getInt("nFaces") == 0) )
      {
	//std::cout << "Inserting before " << *boost::get<std::string>(&(*(i-1))) << std::endl;
	j=i-1;
	break;
      }
    }
  }
  j = bl.insert( j, OFDictData::data(patchName) );
  bl.insert( j+1, bndsubd );
  
  OFDictData::dict::iterator oe=boundaryDict.begin();
  std::swap( boundaryDict[lexical_cast<std::string>(bl.size()/2)], oe->second );
  boundaryDict.erase(oe);
}

bool BoundaryCondition::providesBCsForPatch(const std::string& patchName) const
{
  return (patchName == patchName_);
}




SolverOutputAnalyzer::SolverOutputAnalyzer(ProgressDisplayer& pdisp)
: pdisp_(pdisp),
  curTime_(nan("NAN")),
  curforcename_("")
{
}

void SolverOutputAnalyzer::update(const string& line)
{
  boost::smatch match;
  if (!curforcename_.empty())
  {
    boost::regex sw_pattern("^ *sum of moments:$");
    if ( boost::regex_search( line, match, sw_pattern, boost::match_default ) )
    {
      curforcesection_=2;
    }
    
    boost::regex p_pattern("^ *pressure *: *\\((.*) (.*) (.*)\\)$");
    boost::regex v_pattern("^ *viscous *: *\\((.*) (.*) (.*)\\)$");
    boost::regex por_pattern("^ *porous *: *\\((.*) (.*) (.*)\\)$");
    if ( boost::regex_search( line, match, p_pattern, boost::match_default ) )
    {
      double px=lexical_cast<double>(match[1]);
      double py=lexical_cast<double>(match[2]);
      double pz=lexical_cast<double>(match[3]);
//       std::cout<<"pres ("<<curforcesection_<<") : "<<px<<" "<<py<<" "<<pz<<std::endl;
      if (curforcesection_==1)
      {
	// force
	curforcevalue_(0)=px;
	curforcevalue_(1)=py;
	curforcevalue_(2)=pz;
      }
      else if (curforcesection_==2)
      {
	// moment
	curforcevalue_(6)=px;
	curforcevalue_(7)=py;
	curforcevalue_(8)=pz;
      }
    }
    else if ( boost::regex_search( line, match, v_pattern, boost::match_default ) )
    {
      double vx=lexical_cast<double>(match[1]);
      double vy=lexical_cast<double>(match[2]);
      double vz=lexical_cast<double>(match[3]);
//       std::cout<<"pres ("<<curforcesection_<<") : "<<vx<<" "<<vy<<" "<<vz<<std::endl;
      if (curforcesection_==1)
      {
	// force
	curforcevalue_(3)=vx;
	curforcevalue_(4)=vy;
	curforcevalue_(5)=vz;
      }
      else if (curforcesection_==2)
      {
	// moment
	curforcevalue_(9)=vx;
	curforcevalue_(10)=vy;
	curforcevalue_(11)=vz;
      }
    }
    else if ( boost::regex_search( line, match, por_pattern, boost::match_default ) )
    {
      //
//       std::cout<<"por ("<<curforcesection_<<") "<<std::endl;
      
      if (curforcesection_==2)
      {
	std::cout<<"force="<<curforcevalue_<<std::endl;
	
	// store
	curProgVars_[curforcename_+"_fpx"]=curforcevalue_(0);
	curProgVars_[curforcename_+"_fpy"]=curforcevalue_(1);
	curProgVars_[curforcename_+"_fpz"]=curforcevalue_(2);
	curProgVars_[curforcename_+"_fvx"]=curforcevalue_(3);
	curProgVars_[curforcename_+"_fvy"]=curforcevalue_(4);
	curProgVars_[curforcename_+"_fvz"]=curforcevalue_(5);
	curProgVars_[curforcename_+"_mpx"]=curforcevalue_(6);
	curProgVars_[curforcename_+"_mpy"]=curforcevalue_(7);
	curProgVars_[curforcename_+"_mpz"]=curforcevalue_(8);
	curProgVars_[curforcename_+"_mvx"]=curforcevalue_(9);
	curProgVars_[curforcename_+"_mvy"]=curforcevalue_(10);
	curProgVars_[curforcename_+"_mvz"]=curforcevalue_(11);
	
	// reset tracker
	curforcename_="";
      }
    }
  }
  else
  {
    boost::regex time_pattern("^Time = (.+)$");
    boost::regex solver_pattern("^(.+): +Solving for (.+), Initial residual = (.+), Final residual = (.+), No Iterations (.+)$");
    boost::regex cont_pattern("^time step continuity errors : sum local = (.+), global = (.+), cumulative = (.+)$");
    boost::regex force_pattern("^forces (.+) output:$");

    if ( boost::regex_search( line, match, force_pattern, boost::match_default ) )
    {
      std::cout<<"force output recog"<<std::endl;
      curforcename_=match[1];
      curforcesection_=1;
      curforcevalue_=arma::zeros(12);
    }
    else if ( boost::regex_search( line, match, time_pattern, boost::match_default ) )
    {
      if (curTime_ == curTime_)
      {
	pdisp_.update( ProgressState(curTime_, curProgVars_));
	curProgVars_.clear();
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
}

bool SolverOutputAnalyzer::stopRun() const 
{ 
  return pdisp_.stopRun(); 
}

const OFDictData::dimensionSet dimPressure = OFDictData::dimension(1, -1, -2, 0, 0, 0, 0);
const OFDictData::dimensionSet dimKinPressure = OFDictData::dimension(0, 2, -2, 0, 0, 0, 0);
const OFDictData::dimensionSet dimKinEnergy = OFDictData::dimension(0, 2, -2, 0, 0, 0, 0);
const OFDictData::dimensionSet dimVelocity = OFDictData::dimension(0, 1, -1, 0, 0, 0, 0);
const OFDictData::dimensionSet dimLength = OFDictData::dimension(0, 1, 0, 0, 0, 0, 0);
const OFDictData::dimensionSet dimDensity = OFDictData::dimension(1, -3, 0, 0, 0, 0, 0);
const OFDictData::dimensionSet dimless = OFDictData::dimension(0, 0, 0, 0, 0, 0, 0);
const OFDictData::dimensionSet dimKinViscosity = OFDictData::dimension(0, 2, -1, 0, 0, 0, 0);
const OFDictData::dimensionSet dimDynViscosity = OFDictData::dimension(1, -1, -1, 0, 0, 0, 0);
const OFDictData::dimensionSet dimTemperature = OFDictData::dimension(0, 0, 0, 1, 0, 0, 0);
const OFDictData::dimensionSet dimCurrent = OFDictData::dimension(0, 0, 0, 0, 0, 1, 0);

bool OpenFOAMCase::isCompressible() const
{
  return findUniqueElement<FVNumerics>().isCompressible();
}

boost::shared_ptr<OFdicts> OpenFOAMCase::createDictionaries() const
{
  boost::shared_ptr<OFdicts> dictionaries(new OFdicts);

  // populate fields list
  createFieldListIfRequired();
  
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
	 if (e)
	 {
	   e->addIntoDictionaries(*dictionaries);
	 }
       }  

  return dictionaries;
}

void OpenFOAMCase::modifyMeshOnDisk(const boost::filesystem::path& location) const
{
  for (boost::ptr_vector<CaseElement>::const_iterator i=elements_.begin();
      i!=elements_.end(); i++)
  {
    const OpenFOAMCaseElement *e= dynamic_cast<const OpenFOAMCaseElement*>(&(*i));
    if (e)
    {
      e->modifyMeshOnDisk(*this, location);
    }
  }
}


void OpenFOAMCase::modifyCaseOnDisk(const boost::filesystem::path& location) const
{
  for (boost::ptr_vector<CaseElement>::const_iterator i=elements_.begin();
      i!=elements_.end(); i++)
  {
    const OpenFOAMCaseElement *e= dynamic_cast<const OpenFOAMCaseElement*>(&(*i));
    if (e)
    {
      e->modifyCaseOnDisk(*this, location);
    }
  }
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
    boost::filesystem::path dictpath = basepath / i->first;
    if (!exists(dictpath.parent_path())) 
    {
      boost::filesystem::create_directories(dictpath.parent_path());
    }
    
    {
      std::ofstream f(dictpath.c_str());
      i->second->write(dictpath);
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

bool OpenFOAMCase::outputTimesPresentOnDisk( const boost::filesystem::path& location, bool checkpar ) const
{
  TimeDirectoryList timedirs;
  if (checkpar)
  {
    boost::filesystem::path pd=location/"processor0";
    if (!boost::filesystem::exists(pd)) return false;
    timedirs=listTimeDirectories(pd);
  }
  else
    timedirs=listTimeDirectories(location);
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


void OpenFOAMCase::setFromXML(const std::string& contents, const boost::filesystem::path& file, bool skipOFE, bool skipBCs, const boost::filesystem::path& casepath)
{
  using namespace rapidxml;
  
  xml_document<> doc;
  doc.parse<0> ( const_cast<char*>(&contents[0]) );

  xml_node<> *rootnode = doc.first_node ( "root" );

  if (!skipOFE)
  {
    xml_node<> *OFEnode = rootnode->first_node ( "OFE" );
    if ( OFEnode )
      {
        std::string name = OFEnode->first_attribute ( "name" )->value();
        env_=insight::OFEs::get(name.c_str());
      }
  }

  for ( xml_node<> *e = rootnode->first_node ( "OpenFOAMCaseElement" ); e; e = e->next_sibling ( "OpenFOAMCaseElement" ) )
    {
      std::string type_name = e->first_attribute ( "type" )->value();

      ParameterSet cp = OpenFOAMCaseElement::defaultParameters(type_name);
      cp.readFromNode ( doc, *e, file.parent_path() );
      this->insert(OpenFOAMCaseElement::lookup(type_name, *this, cp));
    }

  if ( !skipBCs )
    {
        insight::OFDictData::dict boundaryDict;
        
      xml_node<> *BCnode = rootnode->first_node ( "BoundaryConditions" );
      if ( BCnode )
        {
          bool bdp=true;
          try
            {
              // parse boundary information
              this->parseBoundaryDict ( casepath, boundaryDict );
            }
          catch ( ... )
            {
              bdp=false;
            }

          if ( bdp )
            {
              xml_node<> *unassignedBCnode = BCnode->first_node ( "UnassignedPatches" );
              std::string def_bc_type = unassignedBCnode->first_attribute ( "BCtype" )->value();
              ParameterSet defp;
              if ( def_bc_type!="" )
                {
                  defp = BoundaryCondition::defaultParameters ( def_bc_type );
                }

              for ( xml_node<> *e = BCnode->first_node ( "Patch" ); e; e = e->next_sibling ( "Patch" ) )
                {
                  std::string patch_name = e->first_attribute ( "patchName" )->value();
                  std::string bc_type = e->first_attribute ( "BCtype" )->value();
                  if ( bc_type!="" )
                    {
                      ParameterSet curp = BoundaryCondition::defaultParameters ( bc_type );
                      curp.readFromNode ( doc, *e, file.parent_path() );
                      this->insert ( insight::BoundaryCondition::lookup ( bc_type, *this, patch_name, boundaryDict, curp ) );
                    }
                }
              if ( def_bc_type!="" )
                {
                  this->addRemainingBCs ( def_bc_type, boundaryDict, defp );
                }
            }
          else
            {
              insight::Warning("The boundary file could not be parsed! Skipping BC configuration." );
            }
        }
    }
}


void OpenFOAMCase::loadFromFile(const boost::filesystem::path& filename, bool skipOFE, bool skipBCs, const boost::filesystem::path& casepath)
{
    if (!boost::filesystem::exists(filename))
        throw insight::Exception("Input file "+filename.string()+" does not exist!");
    
    std::ifstream in(filename.c_str());
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    
    setFromXML(contents, filename, skipOFE, skipBCs, casepath);
}


std::vector< string > OpenFOAMCase::fieldNames() const
{
    createFieldListIfRequired();
  std::vector<std::string> fns;
  BOOST_FOREACH(const FieldList::value_type& v, fields_)
  {
    fns.push_back(v.first);
  }
  return fns;
}


void OpenFOAMCase::createFieldListIfRequired() const
{
  if ( !fieldListCompleted_ )
    {
      FieldList& fields = const_cast<FieldList&> ( fields_ );

      for ( boost::ptr_vector<CaseElement>::const_iterator i=elements_.begin();
            i!=elements_.end(); i++ )
        {
          const OpenFOAMCaseElement *e= dynamic_cast<const OpenFOAMCaseElement*> ( & ( *i ) );
          if ( e )
            {
              e->addFields ( const_cast<OpenFOAMCase&> ( *this ) );
            }
        }
      const_cast<bool&>(fieldListCompleted_)=true;
    }
}

OpenFOAMCase::OpenFOAMCase(const OFEnvironment& env)
: Case(),
  env_(env),
  requiredMapMethod_(directMapMethod)
{
}

OpenFOAMCase::OpenFOAMCase(const OpenFOAMCase& other)
: Case(other),
  env_(other.env_),
  requiredMapMethod_(other.requiredMapMethod_)
{
}

OpenFOAMCase::~OpenFOAMCase()
{
}

void OpenFOAMCase::addField(const std::string& name, const FieldInfo& field)
{
    if (fieldListCompleted_)
        throw insight::Exception("Internal error: tryed to add field "+name+" after field list was already build!");
    
  fields_[name]=field;
}

void OpenFOAMCase::parseBoundaryDict(const boost::filesystem::path& location, OFDictData::dict& boundaryDict) const
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
  std::string shellcmd="";
  
  shellcmd += 
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
    
    if (analyzer.stopRun())
    {
      std::ofstream f( (location/"wnowandstop").c_str() );
      f<<"STOP"<<std::endl;
    }
    
    boost::this_thread::interruption_point();
    if (stopFlag) { if (*stopFlag) break; }
  }
  p_in.close();

  if (p_in.rdbuf()->status()!=0)
    throw insight::Exception("OpenFOAMCase::runSolver(): solver execution failed with nonzero exit code!");
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

void OpenFOAMCase::addRemainingBCs ( const std::string& bc_type, OFDictData::dict& boundaryDict, const ParameterSet& ps )
{
    typedef std::set<std::string> StringSet;
    StringSet unhandledPatches = getUnhandledPatches ( boundaryDict );

    for ( StringSet::const_iterator i=unhandledPatches.begin(); i!=unhandledPatches.end(); i++ ) {
        insert ( BoundaryCondition::lookup ( bc_type, *this, *i, boundaryDict, ps ) );
    }
}

}
