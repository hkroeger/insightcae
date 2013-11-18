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

using namespace rapidxml;

namespace insight
{

ParameterSet::ParameterSet()
{
}

ParameterSet::ParameterSet(const EntryList& entries)
{
  BOOST_FOREACH( const ParameterSet::SingleEntry& i, entries )
  {
    std::string key(boost::get<0>(i));
    insert(key, boost::get<1>(i));
  }
}

ParameterSet::~ParameterSet()
{
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
      +"{\\bf "+i->first+"} = "
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

void ParameterSet::saveToFile(const boost::filesystem::path& file) const
{
  xml_document<> doc;
  
  // xml declaration
  xml_node<>* decl = doc.allocate_node(node_declaration);
  decl->append_attribute(doc.allocate_attribute("version", "1.0"));
  decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
  doc.append_node(decl);

  xml_node<> *rootnode = doc.allocate_node(node_element, "root");
  doc.append_node(rootnode);
  
  appendToNode(doc, *rootnode);
  
  std::ofstream f(file.c_str());
  f << doc << std::endl;
}

void ParameterSet::readFromFile(const boost::filesystem::path& file)
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
  
  readFromNode(doc, *rootnode);
}


SubsetParameter::SubsetParameter(const ParameterSet& defaultValue, const std::string& description)
: Parameter(description),
  value_(defaultValue.clone())
{
}

std::string SubsetParameter::latexRepresentation() const
{
  return value_->latexRepresentation();
}


Parameter* SubsetParameter::clone() const
{
  return new SubsetParameter(*value_, description_);
}

void SubsetParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const
{
  std::cout<<"appending subset "<<name<<std::endl;
  using namespace rapidxml;
  xml_node<>* child = doc.allocate_node(node_element, "subset");
  node.append_node(child);
  child->append_attribute(doc.allocate_attribute
  (
    "name", 
    doc.allocate_string(name.c_str()))
  );
  value_->appendToNode(doc, *child);
}

void SubsetParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, "subset", name);
  value_->readFromNode(doc, *child);
}

}
