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
      cout<<"merging subdict "<<key<<endl;
      SubsetParameter *myp = dynamic_cast<SubsetParameter*>( this->find(key)->second );
      myp->merge(*p);
      delete p;
    }
    else insert(key, boost::get<1>(i)); // take ownership of objects in given list!
  }
}

ParameterSet& ParameterSet::getSubset(const std::string& name) 
{ 
  if (name==".")
    return *this;
  else
    return this->get<SubsetParameter>(name)();
}

const ParameterSet& ParameterSet::getSubset(const std::string& name) const
{
  if (name==".")
    return *this;
  else
    return this->get<SubsetParameter>(name)();
}

std::string ParameterSet::latexRepresentation() const
{
  std::string result= 
  "\\begin{enumerate}\n";
  for(const_iterator i=begin(); i!=end(); i++)
  {
    result+=
    "\\item "
      +cleanSymbols(i->second->description())
      +"\\\\\n"
      +"{\\bf "+cleanSymbols(i->first)+"} = "
      +i->second->latexRepresentation()
      +"\n";
  }
  result+="\\end{enumerate}\n";
 
  return result;
}

ParameterSet* ParameterSet::clone() const
{
  ParameterSet *np=new ParameterSet;
  for (ParameterSet::const_iterator i=begin(); i!=end(); i++)
  {
    std::string key(i->first);
    np->insert(key, i->second->clone());
  }
  return np;
}

void ParameterSet::appendToNode(rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const
{
  for( const_iterator i=begin(); i!= end(); i++)
  {
    i->second->appendToNode(i->first, doc, node);
  }
}

void ParameterSet::readFromNode(rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node)
{
  for( iterator i=begin(); i!= end(); i++)
  {
    i->second->readFromNode(i->first, doc, node);
  }
}

void ParameterSet::saveToFile(const boost::filesystem::path& file, std::string analysisName ) const
{
  std::cout<<"Writing parameterset to file "<<file<<std::endl;
  
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

  appendToNode(doc, *rootnode);
  
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
  
  readFromNode(doc, *rootnode);
  
  return analysisName;
}

defineType(SubsetParameter);
addToFactoryTable(Parameter, SubsetParameter, std::string);


SubsetParameter::SubsetParameter(const std::string& description)
: Parameter(description)
{
}

SubsetParameter::SubsetParameter(const ParameterSet& defaultValue, const std::string& description)
: Parameter(description),
  value_(defaultValue.clone())
{
}

void SubsetParameter::merge(const SubsetParameter& other)
{
  value_->extend(other.value_->entries());
}

std::string SubsetParameter::latexRepresentation() const
{
  return value_->latexRepresentation();
}


Parameter* SubsetParameter::clone() const
{
  return new SubsetParameter(*value_, description_);
}

rapidxml::xml_node<>* SubsetParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const
{
  std::cout<<"appending subset "<<name<<std::endl;
  using namespace rapidxml;
  xml_node<>* child = Parameter::appendToNode(name, doc, node);
  value_->appendToNode(doc, *child);
  return child;
}

void SubsetParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name);
  if (child)
    value_->readFromNode(doc, *child);
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
  return "(Not implemented)";
}

Parameter* SelectableSubsetParameter::clone () const
{
  SelectableSubsetParameter *np=new SelectableSubsetParameter(description_);
  np->selection_=selection_;
  for (ItemList::const_iterator i=value_.begin(); i!=value_.end(); i++)
  {
    std::string key(i->first);
    np->value_.insert(key, i->second->clone());
  }
  return np; 
}

rapidxml::xml_node<>* SelectableSubsetParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const
{
  using namespace rapidxml;
  
   xml_node<>* child = Parameter::appendToNode(name, doc, node);
//   child->append_attribute(doc.allocate_attribute
//   (
//     "value", 
//     doc.allocate_string(valueToString(value_).c_str())
//   ));
//     
//   std::cout<<"appending subset "<<name<<std::endl;
//   using namespace rapidxml;
//   xml_node<>* child = Parameter::appendToNode(name, doc, node);
//   value_->appendToNode(doc, *child);
   return child;  
}

void SelectableSubsetParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node)
{
}

}

