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


#include "parameterset.h"
#include "base/parameters.h"
#include "base/latextools.h"
#include "base/tools.h"

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include "boost/foreach.hpp"

#include <fstream>
#include <numeric>

using namespace std;
using namespace rapidxml;

namespace insight
{


SubParameterSet::~SubParameterSet()
{}

ParameterSet &SubParameterSet::subsetRef()
{
  return const_cast<ParameterSet&>( const_cast<SubParameterSet*>(this)->subset() );
}





ParameterSet::ParameterSet()
{
}

ParameterSet::ParameterSet(const ParameterSet& o)
//: boost::ptr_map<std::string, Parameter>(o.clone())
: std::map<std::string, std::unique_ptr<Parameter> >()
{
  operator=(o);
}

ParameterSet::ParameterSet(const EntryList& entries)
{
  extend(entries);
}

ParameterSet::~ParameterSet()
{
}

bool ParameterSet::isDifferent(const ParameterSet &op) const
{
  if (op.size()!=this->size())
    return true;

  auto thisi=this->begin(), opi=op.begin();
  while ( (thisi!=this->end()) && (opi!=op.end()) )
  {
    if ( thisi->first != opi->first )
      return true;

    if ( thisi->second->isDifferent( *(opi->second) ) )
      return true;

    ++thisi; ++opi;
  }

  return false;
}

void ParameterSet::setParameterSetDescription(const std::string& desc)
{
  parameterSetDescription_.simpleLatex() = desc;
}

const SimpleLatex& ParameterSet::parameterSetDescription() const
{
  return parameterSetDescription_;
}


void ParameterSet::operator=(const ParameterSet& o)
{
  clear();
  std::transform(
        o.begin(), o.end(),
        std::inserter(*this, end()),
        [](const value_type& op)
        {
          return value_type(
                op.first,
                std::unique_ptr<Parameter>(op.second->clone()) );
        }
  );
}

ParameterSet::EntryList ParameterSet::entries() const
{
  EntryList alle;
  for ( const value_type& e: *this)
  {
    alle.push_back(SingleEntry(e.first, e.second->clone()));
  }
  return alle;
}

void ParameterSet::extend(const EntryList& entries)
{
  for ( const ParameterSet::SingleEntry& i: entries )
  {
    std::string key(boost::get<0>(i));
    SubParameterSet *p = dynamic_cast<SubParameterSet*>( boost::get<1>(i) );
    if (p && this->contains(key))
    {
      // if current parameter to insert is a subset and is already existing in current set...
        SubParameterSet *myp = dynamic_cast<SubParameterSet*>( this->find(key)->second.get() );
        myp->subsetRef().extend(p->subset().entries());
	delete p;
    }
    else if (!this->contains(key))
    {
      // otherwise, append, if key is not existing
      // note: insert does not replace! insertion will be omitted, if key exists already
      insert( value_type(key, std::unique_ptr<Parameter>(boost::get<1>(i))) ); // take ownership of objects in given list!
    }
  }
}

ParameterSet& ParameterSet::merge(const ParameterSet& other, bool allowInsertion )
{
  EntryList entries=other.entries();
  for ( const ParameterSet::SingleEntry& i: entries )
  {
    std::string key(boost::get<0>(i));
    if (this->contains(key))
    {
      if (auto *p = dynamic_cast<SubParameterSet*>( boost::get<1>(i) ))
      {
        // merging subdict
        SubParameterSet *myp = dynamic_cast<SubParameterSet*>( this->find(key)->second.get() );
        myp->merge(*p, allowInsertion);
        delete p;
      }
      else
      {
        // replacing
        replace(key, boost::get<1>(i)); // take ownership of objects in given list!
      }
    }
    else 
    {
      if (allowInsertion)
      {
          // inserting
          insert( value_type(key, std::unique_ptr<Parameter>(boost::get<1>(i))) ); // take ownership of objects in given list!
      }
    }
  }

  return *this;
}



ParameterSet ParameterSet::intersection(const ParameterSet &other) const
{
  EntryList entries;
  for ( const ParameterSet::SingleEntry& i: other.entries() )
  {
    std::string otherkey = boost::get<0>(i);
    Parameter *otherParameter = boost::get<1>(i);

    if (this->contains(otherkey)
        &&
        this->get<Parameter>(otherkey).type()==otherParameter->type() )
    {
      auto *myp = this->find(otherkey)->second.get();

      if (auto *op =
          dynamic_cast<SubParameterSet*>( otherParameter ))
      {
        // intersect subdict
        auto *mysd = dynamic_cast<SubParameterSet*>( myp );

        entries.push_back({ otherkey, mysd->intersection(*op) });
      }
      else
      {
        // replacing
        entries.push_back({ otherkey, myp->clone() });
      }
    }
  }

  return ParameterSet(entries);
}







std::string splitOffFirstParameter(std::string& path, int& nRemaining)
{
  using namespace boost;
  using namespace boost::algorithm;

  if ( boost::contains ( path, "/" ) )
  {
    std::string prefix = copy_range<std::string> ( *make_split_iterator ( path, first_finder ( "/" ) ) );

    std::string remain = path;
    erase_head ( remain, prefix.size()+1 );

    path=remain;
    nRemaining = std::count(path.begin(), path.end(), '/')+1;
    return prefix;
  }
  else
  {
    std::string prefix=path;
    path="";
    nRemaining=0;
    return prefix;
  }
}




bool ParameterSet::hasParameter(std::string path) const
{
  using namespace boost;
  using namespace boost::algorithm;

  int nRemaining=-1;
  std::string parameterName = splitOffFirstParameter(path, nRemaining);

  insight::CurrentExceptionContext ex("checking existence of parameter "+parameterName, false);

  auto parameter = find(parameterName);

  if (parameter == end())
  {
    return false;
  }

  if (nRemaining == 0)
  {
    return true;
  }
  else
  {
    SubParameterSet* sps = nullptr;

    if (! (sps = dynamic_cast<insight::SubParameterSet*>(parameter->second.get())))
    {
      if (auto* ap = dynamic_cast<insight::ArrayParameter*>(parameter->second.get()))
      {
        std::string indexString = splitOffFirstParameter(path, nRemaining);

        insight::Parameter* arrayElement=nullptr;

        if (indexString=="default")
        {
            arrayElement=&const_cast<Parameter&>(ap->defaultValue());
        }
        else
        {
            int i = toNumber<int>(indexString);

            if ( (i<0) || (i>=ap->size()) )
                return false;

            arrayElement = &(*ap)[i];
        }

        if (nRemaining==0)
        {
            return true;
        }
        else // nRemaining >0
        {
            sps = dynamic_cast<SubParameterSet*>(arrayElement);
        }
      }
    }

    if (sps)
    {
      return sps->subsetRef().hasParameter(path);
    }
    else
    {
      return false;
    }
  }
}


Parameter &ParameterSet::getParameter(std::string path)
{
  using namespace boost;
  using namespace boost::algorithm;

  int nRemaining=-1;
  std::string parameterName = splitOffFirstParameter(path, nRemaining);

  insight::CurrentExceptionContext ex("looking up parameter "+parameterName, false);

  auto parameter = find(parameterName);

  if (parameter == end())
  {
    throw insight::Exception("There is no parameter with name "+parameterName);
  }

  if (nRemaining == 0)
  {
    return *parameter->second;
  }
  else
  {
    SubParameterSet* sps = nullptr;

    if (! (sps = dynamic_cast<insight::SubParameterSet*>(parameter->second.get())))
    {
      if (auto* ap = dynamic_cast<insight::ArrayParameter*>(parameter->second.get()))
      {
        std::string indexString = splitOffFirstParameter(path, nRemaining);

        insight::Parameter* arrayElement=nullptr;

        if (indexString=="default")
        {
          arrayElement=&const_cast<Parameter&>(ap->defaultValue());
        }
        else
        {
          int i = toNumber<int>(indexString);

          if ( (i<0) || (i>=ap->size()) )
            throw insight::Exception(
                str(format("requested array index %d beyond array bounds (size %d)") % i % ap->size())
                );

          arrayElement = &(*ap)[i];
        }

        if (nRemaining==0)
        {
          return *arrayElement;
        }
        else // nRemaining >0
        {
          sps = dynamic_cast<SubParameterSet*>(arrayElement);
        }
      }
    }

    if (sps)
    {
        return sps->subsetRef().getParameter(path);
    }
    else
    {
        throw insight::Exception(
            "cannot lookup subpath "+path+" because parameter "
            +parameterName+" is not a sub dictionary."
            );
    }
  }
}


int& ParameterSet::getInt ( const std::string& name )
{
  return this->get<IntParameter> ( name ) ();
}

double& ParameterSet::getDouble ( const std::string& name )
{
  return this->get<DoubleParameter> ( name ) ();
}

bool& ParameterSet::getBool ( const std::string& name )
{
  return this->get<BoolParameter> ( name ) ();
}

std::string& ParameterSet::getString ( const std::string& name )
{
  return this->get<StringParameter> ( name ) ();
}

arma::mat& ParameterSet::getVector ( const std::string& name )
{
  return this->get<VectorParameter> ( name ) ();
}

arma::mat& ParameterSet::getMatrix ( const std::string& name )
{
  return this->get<MatrixParameter> ( name ) ();
}

std::istream& ParameterSet::getFileStream ( const std::string& name )
{
  return this->get<PathParameter> ( name ) .stream();
}

ParameterSet& ParameterSet::setInt ( const std::string& name, int v )
{
  this->get<IntParameter> ( name ) () = v;
  return *this;
}

ParameterSet& ParameterSet::setDouble ( const std::string& name, double v )
{
  this->get<DoubleParameter> ( name ) () = v;
  return *this;
}

ParameterSet& ParameterSet::setBool ( const std::string& name, bool v )
{
  this->get<BoolParameter> ( name ) () = v;
  return *this;
}

ParameterSet& ParameterSet::setString ( const std::string& name, const std::string& v )
{
  this->get<StringParameter> ( name ) () = v;
  return *this;
}

ParameterSet& ParameterSet::setVector ( const std::string& name, const arma::mat& v )
{
  this->get<VectorParameter> ( name ) () = v;
  return *this;
}

ParameterSet& ParameterSet::setMatrix ( const std::string& name, const arma::mat& m )
{
  this->get<MatrixParameter> ( name ) () = m;
  return *this;
}

ParameterSet& ParameterSet::setOriginalFileName ( const std::string& name, const boost::filesystem::path& fp)
{
  this->get<PathParameter> ( name ).setOriginalFilePath(fp);
  return *this;
}


const int& ParameterSet::getInt ( const std::string& name ) const
{
  return this->get<IntParameter> ( name ) ();
}

const double& ParameterSet::getDouble ( const std::string& name ) const
{
  return this->get<DoubleParameter> ( name ) ();
}

const bool& ParameterSet::getBool ( const std::string& name ) const
{
  return this->get<BoolParameter> ( name ) ();
}

const std::string& ParameterSet::getString ( const std::string& name ) const
{
  return this->get<StringParameter> ( name ) ();
}

const arma::mat& ParameterSet::getVector ( const std::string& name ) const
{
  return this->get<VectorParameter> ( name ) ();
}

const boost::filesystem::path ParameterSet::getPath ( const std::string& name, const boost::filesystem::path& basePath ) const
{
  return this->get<PathParameter> ( name ) .filePath(basePath);
}

ParameterSet& ParameterSet::getSubset(const std::string& name) 
{ 
  if (name==".")
    return *this;
  else
  {
    return this->get<SubParameterSet>(name).subsetRef();
  }
}

const ParameterSet& ParameterSet::operator[] ( const std::string& name ) const
{
  return getSubset ( name );
}


const ParameterSet& ParameterSet::getSubset(const std::string& name) const
{
  if (name==".")
    return *this;
  else
  {
    return this->get<SubParameterSet>(name).subset();
  }
}

std::string ParameterSet::latexRepresentation() const
{
  std::string result="";
  if (size()>0)
  {
    result= 
    "\\begin{enumerate}\n";
    for(const_iterator i=begin(); i!=end(); i++)
    {
      auto ldesc=i->second->description().toLaTeX();

      result+="\\item ";
      if (!ldesc.empty())
        result+=ldesc+"\\\\";
      result+=
          "\n"
          "\\textbf{"+SimpleLatex(i->first).toLaTeX()+"} = "
          +i->second->latexRepresentation()
          +"\n";
    }
    result+="\\end{enumerate}\n";
  }
  return result;
}

std::string ParameterSet::plainTextRepresentation(int indent) const
{
    std::string result="";
    if (size()>0)
    {
      for(const_iterator i=begin(); i!=end(); i++)
      {
        result+=std::string(indent, ' ') + (i->first) + " = " + i->second->plainTextRepresentation(indent) + '\n';
      }
    }
    return result;
}


ParameterSet* ParameterSet::cloneParameterSet() const
{
  ParameterSet *np=new ParameterSet;
  for (ParameterSet::const_iterator i=begin(); i!=end(); i++)
  {
    std::string key(i->first);
    np->insert( value_type(key, std::unique_ptr<Parameter>(i->second->clone())) );
  }
  return np;
}

void ParameterSet::appendToNode(rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const
{
  for( const_iterator i=begin(); i!= end(); i++)
  {
    i->second->appendToNode(i->first, doc, node, inputfilepath);
  }
}

void ParameterSet::readFromNode(
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath)
{
  for( iterator i=begin(); i!= end(); i++)
  {
    i->second->readFromNode(i->first, doc, node, inputfilepath);
  }
}


void ParameterSet::packExternalFiles()
{
  CurrentExceptionContext ex("packing external files into parameter set");
  for (auto& p: *this)
  {
    p.second->pack();
  }
}

void ParameterSet::removePackedData()
{
  for (auto& p: *this)
  {
    p.second->clearPackedData();
  }
}

void ParameterSet::unpackAllExternalFiles(const boost::filesystem::path& basePath)
{
    CurrentExceptionContext ex("unpacking external files from parameter set");
    for (auto& p: *this)
    {
      p.second->unpack(basePath);
    }
}


void ParameterSet::saveToStream(std::ostream& os, const boost::filesystem::path& parent_path, std::string analysisName ) const
{
  CurrentExceptionContext ex("writing parameter set content into output stream (parent path "+parent_path.string()+", analysis name "+analysisName);
//   std::cout<<"Writing parameterset to file "<<file<<std::endl;


  // prepare XML document
  xml_document<> doc;
  xml_node<>* decl = doc.allocate_node(node_declaration);
  decl->append_attribute(doc.allocate_attribute("version", "1.0"));
  decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
  doc.append_node(decl);
  xml_node<> *rootnode = doc.allocate_node(node_element, "root");
  doc.append_node(rootnode);

  // insert analysis name
  if (analysisName != "")
  {
    xml_node<> *analysisnamenode = doc.allocate_node(node_element, "analysis");
    rootnode->append_node(analysisnamenode);
    analysisnamenode->append_attribute(doc.allocate_attribute
    (
      "name",
      doc.allocate_string(analysisName.c_str())
    ));
  }

  // store parameters
  appendToNode(doc, *rootnode, parent_path);

  os << doc;
}

void ParameterSet::saveToFile(const boost::filesystem::path& file, std::string analysisName ) const
{
    CurrentExceptionContext ex("writing parameter set to file "+file.string());
    std::ofstream f(file.c_str());
    saveToStream( f, file.parent_path(), analysisName );
    f << std::endl;
    f << std::flush;
    f.close();
}


void ParameterSet::saveToString(std::string &s, const boost::filesystem::path& file, std::string analysisType) const
{
    std::ostringstream os(s, std::ios_base::ate);
    saveToStream(os, file.parent_path(), analysisType);
    s = os.str();
}


std::string ParameterSet::readFromFile(const boost::filesystem::path& file, const std::string& startAtSubnode)
{
  CurrentExceptionContext ex("reading parameter set from file "+file.string());

  std::string contents;
  readFileIntoString(file, contents);

  xml_document<> doc;
  doc.parse<0>(&contents[0]);
  
  xml_node<> *rootnode = doc.first_node("root");
  
  std::string analysisName;
  xml_node<> *analysisnamenode = rootnode->first_node("analysis");
  if (analysisnamenode)
  {
    analysisName = analysisnamenode->first_attribute("name")->value();
  }

  if (!startAtSubnode.empty())
  {
      std::vector<std::string> path;
      boost::split(path, startAtSubnode, boost::is_any_of("/"));
      for (const auto& p: path)
      {
          std::map<std::string, xml_node<>*> nodes;
          for (auto *e = rootnode->first_node(); e!=nullptr; e=e->next_sibling())
          {
              nodes[ e->first_attribute("name")->value() ]=e;
          }

          auto e = nodes.find(p);
          if (e==nodes.end())
          {
              std::ostringstream os;
              for(auto& n: nodes) os<<" "<<n.first;
              throw insight::Exception("Could not find node "+p+" (full path "+startAtSubnode+")!\n"
                                       "Available:"+os.str());
          }
          else
          {
              rootnode=e->second;
          }
      }
  }
  
  readFromNode(doc, *rootnode, file.parent_path());
  
  return analysisName;
}





std::ostream& operator<<(std::ostream& os, const ParameterSet& ps)
{
  CurrentExceptionContext ex("writing plain text representation of parameter set to output stream (via << operator)");
  os << ps.plainTextRepresentation(0);
  return os;
}













ParameterSet_Validator::~ParameterSet_Validator()
{}

void ParameterSet_Validator::update(const ParameterSet& ps)
{
    errors_.clear();
    warnings_.clear();
    ps_=ps;
}

bool ParameterSet_Validator::isValid() const
{
    return (warnings_.size()==0) && (errors_.size()==0);
}

const ParameterSet_Validator::WarningList& ParameterSet_Validator::ParameterSet_Validator::warnings() const
{
    return warnings_;
}

const ParameterSet_Validator::ErrorList& ParameterSet_Validator::ParameterSet_Validator::errors() const
{
    return errors_;
}







}

