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
#include "base/latextools.h"

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include "boost/foreach.hpp"

#include <fstream>

using namespace std;
using namespace rapidxml;

namespace insight
{

ParameterSet::ParameterSet()
{
}

ParameterSet::ParameterSet(const ParameterSet& o)
: boost::ptr_map<std::string, Parameter>(o.clone())
{
}

ParameterSet::ParameterSet(const EntryList& entries)
{
  extend(entries);
}

ParameterSet::~ParameterSet()
{
}

ParameterSet::EntryList ParameterSet::entries() const
{
  EntryList alle;
  BOOST_FOREACH( const_iterator::value_type e, *this)
  {
    alle.push_back(SingleEntry(e->first, e->second->clone()));
  }
  return alle;
}

void ParameterSet::extend(const EntryList& entries)
{
  BOOST_FOREACH( const ParameterSet::SingleEntry& i, entries )
  {
    std::string key(boost::get<0>(i));
    SubsetParameter *p = dynamic_cast<SubsetParameter*>( boost::get<1>(i) );
    if (p && this->contains(key))
    {
// 	cout<<"merging subdict "<<key<<endl;
	SubsetParameter *myp = dynamic_cast<SubsetParameter*>( this->find(key)->second );
	myp->extend(p->entries());
	delete p;
    }
    else 
    {
//       cout<<"inserting "<<key<<endl;
    // insert does not replace!
      insert(key, boost::get<1>(i)); // take ownership of objects in given list!
    }
  }
}

void ParameterSet::merge(const ParameterSet& p)
{
  EntryList entries=p.entries();
  BOOST_FOREACH( const ParameterSet::SingleEntry& i, entries )
  {
    std::string key(boost::get<0>(i));
    SubsetParameter *p = dynamic_cast<SubsetParameter*>( boost::get<1>(i) );
    if (this->contains(key))
    {
      if (p)
      {
// 	cout<<"merging subdict "<<key<<endl;
	SubsetParameter *myp = dynamic_cast<SubsetParameter*>( this->find(key)->second );
	myp->merge(*p);
	delete p;
      }
      else
      {
// 	cout<<"replacing"<<key<<endl;
	replace(key, boost::get<1>(i)); // take ownership of objects in given list!
      }
    }
    else 
    {
//       cout<<"inserting "<<key<<endl;
      insert(key, boost::get<1>(i)); // take ownership of objects in given list!
    }
  }
}

ParameterSet& ParameterSet::getSubset(const std::string& name) 
{ 
  if (name==".")
    return *this;
  else
  {
    try
    {
      return this->get<SubsetParameter>(name)();
    }
    catch (...)
    {
      return this->get<SelectableSubsetParameter>(name)();
    }
  }
}

const ParameterSet& ParameterSet::getSubset(const std::string& name) const
{
  if (name==".")
    return *this;
  else
  {
    try
    {
      return this->get<SubsetParameter>(name)();
    }
    catch (...)
    {
      return this->get<SelectableSubsetParameter>(name)();
    }
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
      result+=
      "\\item "
	+cleanSymbols(i->second->description())
	+"\\\\\n"
	+"\\textbf{"+cleanSymbols(i->first)+"} = "
	+i->second->latexRepresentation()
	+"\n";
    }
    result+="\\end{enumerate}\n";
  }
  return result;
}

ParameterSet* ParameterSet::cloneParameterSet() const
{
  ParameterSet *np=new ParameterSet;
  for (ParameterSet::const_iterator i=begin(); i!=end(); i++)
  {
    std::string key(i->first);
    np->insert(key, i->second->clone());
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

void ParameterSet::readFromNode(rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath)
{
  for( iterator i=begin(); i!= end(); i++)
  {
    i->second->readFromNode(i->first, doc, node, inputfilepath);
  }
}

void ParameterSet::saveToFile(const boost::filesystem::path& file, std::string analysisName ) const
{
//   std::cout<<"Writing parameterset to file "<<file<<std::endl;
  
  xml_document<> doc;
  
  // xml declaration
  xml_node<>* decl = doc.allocate_node(node_declaration);
  decl->append_attribute(doc.allocate_attribute("version", "1.0"));
  decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
  doc.append_node(decl);

  xml_node<> *rootnode = doc.allocate_node(node_element, "root");
  doc.append_node(rootnode);
  
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

  appendToNode(doc, *rootnode, file.parent_path());
  
  {
    std::ofstream f(file.c_str());
    f << doc << std::endl;
    f << std::flush;
    f.close();
  }
}

std::string ParameterSet::readFromFile(const boost::filesystem::path& file)
{
  std::ifstream in(file.c_str());
  std::string contents;
  in.seekg(0, std::ios::end);
  contents.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents[0], contents.size());
  in.close();

  xml_document<> doc;
  doc.parse<0>(&contents[0]);
  
  xml_node<> *rootnode = doc.first_node("root");
  
  std::string analysisName;
  xml_node<> *analysisnamenode = rootnode->first_node("analysis");
  if (analysisnamenode)
  {
    analysisName = analysisnamenode->first_attribute("name")->value();
  }
  
  readFromNode(doc, *rootnode, file.parent_path());
  
  return analysisName;
}

defineType(SubsetParameter);
addToFactoryTable(Parameter, SubsetParameter, std::string);


SubsetParameter::SubsetParameter()
{
}

SubsetParameter::SubsetParameter(const std::string& description)
: Parameter(description)
{
}

SubsetParameter::SubsetParameter(const ParameterSet& defaultValue, const std::string& description)
: Parameter(description),
  ParameterSet(defaultValue.entries())
{
}

// void SubsetParameter::merge(const SubsetParameter& other)
// {
//   this->merge(other);
// }

std::string SubsetParameter::latexRepresentation() const
{
  return ParameterSet::latexRepresentation();
}


Parameter* SubsetParameter::clone() const
{
  return new SubsetParameter(*this, description_);
}

rapidxml::xml_node<>* SubsetParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const
{
//   std::cout<<"appending subset "<<name<<std::endl;
  using namespace rapidxml;
  xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
  ParameterSet::appendToNode(doc, *child, inputfilepath);
  return child;
}

void SubsetParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name);
  if (child)
  {
    ParameterSet::readFromNode(doc, *child, inputfilepath);
  }
}

defineType(SelectableSubsetParameter);
addToFactoryTable(Parameter, SelectableSubsetParameter, std::string);

SelectableSubsetParameter::SelectableSubsetParameter(const std::string& description)
: Parameter(description)
{
}

SelectableSubsetParameter::SelectableSubsetParameter(const key_type& defaultSelection, const SubsetList& defaultValue, const std::string& description)
: Parameter(description),
  selection_(defaultSelection)
{
  BOOST_FOREACH( const SelectableSubsetParameter::SingleSubset& i, defaultValue )
  {
    std::string key(boost::get<0>(i));
    value_.insert(key, boost::get<1>(i)); // take ownership of objects in given list!
  }
}

std::string SelectableSubsetParameter::latexRepresentation() const
{
//  return "(Not implemented)";
  std::ostringstream os;
  os<<"selected as ``"<<selection_<<"''\\\\"<<endl;
  os<<operator()().latexRepresentation();
  return os.str();
}

Parameter* SelectableSubsetParameter::clone () const
{
  SelectableSubsetParameter *np=new SelectableSubsetParameter(description_);
  np->selection_=selection_;
  for (ItemList::const_iterator i=value_.begin(); i!=value_.end(); i++)
  {
    std::string key(i->first);
    np->value_.insert(key, i->second->cloneParameterSet());
  }
  return np; 
}

rapidxml::xml_node<>* SelectableSubsetParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const
{
  using namespace rapidxml;
  
  xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
  child->append_attribute(doc.allocate_attribute
  (
    "value", 
    doc.allocate_string(selection_.c_str())
  ));
  
  operator()().appendToNode(doc, *child, inputfilepath);

  return child;  
}

void SelectableSubsetParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name);
  if (child)
  {
    selection_=child->first_attribute("value")->value();
    
    if (value_.find(selection_)==value_.end())
      throw insight::Exception("Invalid selection key during read of selectableSubset "+name);
    
    operator()().readFromNode(doc, *child, inputfilepath);
  }
}

}

