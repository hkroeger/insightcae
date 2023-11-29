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

#include "parameter.h"
#include "base/latextools.h"
#include "base/tools.h"
#include <iostream>

#include <iterator>
#include "boost/algorithm/string/trim.hpp"

#include "base/parameterset.h"
#include "base/rapidxml.h"
#include "rapidxml/rapidxml_print.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace rapidxml;



namespace insight 
{
  
    
rapidxml::xml_node<> *findNode(rapidxml::xml_node<>& father, const std::string& name, const std::string& typeName)
{
    if (name.empty())
    {
        return &father;
    }
    else
    {
        for (xml_node<> *child = father.first_node(typeName.c_str()); child; child = child->next_sibling(typeName.c_str()))
        {
          if (child->first_attribute("name")->value() == name)
          {
            return child;
          }
        }
    }

    return nullptr;
}

    






void writeMatToXMLNode(const arma::mat& matrix, xml_document< char >& doc, xml_node< char >& node)
{
  std::ostringstream voss;
  matrix.save(voss, arma::raw_ascii);
  
  // set stringified table values as node value
  node.value(doc.allocate_string(voss.str().c_str()));
}








ArrayParameterBase::~ArrayParameterBase()
{}

Parameter &ArrayParameterBase::elementRef(int i)
{
  return const_cast<Parameter&>(element(i));
}




defineType(Parameter);
defineFactoryTable(Parameter, LIST(const std::string& desc), LIST(desc) );


Parameter::iterator::iterator()
    : p_(nullptr),
    iChild_(-1)
{}


Parameter::iterator::iterator(Parameter& p, int i)
  : p_(&p),
    iChild_(i)
{}

Parameter::iterator::iterator(const iterator& i)
  : p_(i.p_),
    iChild_(i.iChild_)
{}

Parameter::iterator::~iterator()
{}

Parameter::iterator& Parameter::iterator::operator=(const iterator&o)
{
  auto other=o;
  std::swap(*this, other);
  return *this;
}

bool Parameter::iterator::operator==(const iterator& o) const
{
  return (o.p_==p_) && (o.iChild_==iChild_);
}

bool Parameter::iterator::operator!=(const iterator& o) const
{
  return !operator==(o);
}


Parameter::iterator& Parameter::iterator::operator++()
{
  if (p_)
  {
        iChild_++;
  }
  return *this;
}

Parameter::iterator::reference Parameter::iterator::operator*() const
{
  insight::assertion(
      p_!=nullptr, "invalid iterator (no reference)" );
  insight::assertion(
      iChild_>=0 && iChild_<p_->nChildren(), "invalid iterator (out of range 0..%d)", p_->nChildren()-1 );

  return p_->childParameterRef(iChild_);
}

Parameter::iterator::pointer Parameter::iterator::operator->() const
{
  return &operator*();
}

string Parameter::iterator::name() const
{
  insight::assertion(
      p_!=nullptr, "invalid iterator (no reference)" );
  insight::assertion(
      iChild_>=0 && iChild_<p_->nChildren(), "invalid iterator (out of range 0..%d)", p_->nChildren()-1 );

  return p_->childParameterName(iChild_);
}




Parameter::const_iterator::const_iterator()
  : p_(nullptr),
    iChild_(-1)
{}

Parameter::const_iterator::const_iterator(const Parameter& p, int i)
  : p_(&p),
    iChild_(i)
{}

Parameter::const_iterator::const_iterator(const iterator& i)
  : p_(i.p_),
    iChild_(i.iChild_)
{}

Parameter::const_iterator::const_iterator(const const_iterator& i)
  : p_(i.p_),
    iChild_(i.iChild_)
{}

Parameter::const_iterator::~const_iterator()
{}

Parameter::const_iterator& Parameter::const_iterator::operator=(const const_iterator& o)
{
  auto other=o;
  std::swap(*this, other);
  return *this;
}

bool Parameter::const_iterator::operator==(const const_iterator& o) const
{
  return (o.p_==p_) && (o.iChild_==iChild_);
}

bool Parameter::const_iterator::operator!=(const const_iterator& o) const
{
  return !operator==(o);
}

Parameter::const_iterator& Parameter::const_iterator::operator++()
{
  if (p_)
  {
        iChild_++;
  }
  return *this;
}

Parameter::const_iterator::reference Parameter::const_iterator::operator*() const
{
  insight::assertion(
      p_!=nullptr, "invalid iterator (no reference)" );
  insight::assertion(
      iChild_>=0 && iChild_<p_->nChildren(), "invalid iterator (out of range 0..%d)", p_->nChildren()-1 );

  return p_->childParameter(iChild_);
}

Parameter::const_iterator::pointer Parameter::const_iterator::operator->() const
{
  return &operator*();
}

string Parameter::const_iterator::name() const
{
  insight::assertion(
      p_!=nullptr, "invalid iterator (no reference)" );
  insight::assertion(
      iChild_>=0 && iChild_<p_->nChildren(), "invalid iterator (out of range 0..%d)", p_->nChildren()-1 );

  return p_->childParameterName(iChild_);
}





void Parameter::triggerValueChanged()
{
  if (!valueChangeSignalBlocked_) valueChanged();
}

void Parameter::triggerChildValueChanged()
{
  if (!valueChangeSignalBlocked_) childValueChanged();
}

Parameter::Parameter()
: description_(),
  isHidden_(false), isExpert_(false), isNecessary_(false), order_(0),
  valueChangeSignalBlocked_(false)
{
}

Parameter::Parameter(const std::string& description, bool isHidden, bool isExpert, bool isNecessary, int order)
: description_(description),
  isHidden_(isHidden), isExpert_(isExpert), isNecessary_(isNecessary), order_(order),
  valueChangeSignalBlocked_(false)
{
}

Parameter::~Parameter()
{}


bool Parameter::isHidden() const { return isHidden_; }
bool Parameter::isExpert() const {return isExpert_; }
bool Parameter::isNecessary() const { return isNecessary_; }

bool Parameter::isDifferent(const Parameter &) const
{
  throw insight::Exception("cannot compare parameters: the isDifferent function is not implemented1");
  return true;
}

int Parameter::order() const { return order_; }


rapidxml::xml_node<>* Parameter::appendToNode(
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        boost::filesystem::path ) const
{
  using namespace rapidxml;

  if (name.empty())
  {
        return &node;
  }
  else
  {
        xml_node<>* child = doc.allocate_node(node_element, doc.allocate_string(this->type().c_str()));
        node.append_node(child);
        child->append_attribute(
            doc.allocate_attribute
            (
                "name",
                doc.allocate_string(name.c_str()))
            );

        return child;
  }
}



void Parameter::saveToNode(
    xml_document<>& doc,
    xml_node<>& rootNode,
    const boost::filesystem::path& parent_path,
    std::string analysisName ) const
{
    CurrentExceptionContext ex(
        2,
        "writing parameter %s content into XML node"
        " (parent path %s, analysis name %s",
        type().c_str(), parent_path.string().c_str(), analysisName.c_str());

    // insert analysis name
    if (analysisName != "")
    {
        xml_node<> *analysisnamenode = doc.allocate_node(node_element, "analysis");
        rootNode.append_node(analysisnamenode);
        analysisnamenode->append_attribute(
            doc.allocate_attribute
            (
                "name",
                doc.allocate_string(analysisName.c_str())
                ));
    }

    // store parameters
    appendToNode(std::string(), doc, rootNode, parent_path);
}




void Parameter::saveToStream(
    std::ostream& os,
    const boost::filesystem::path& parent_path,
    std::string analysisName ) const
{
  CurrentExceptionContext ex(2,
      "writing parameter %s content into output stream (parent path %s, analysis name %s",
      type().c_str(), parent_path.string().c_str(), analysisName.c_str());

  // prepare XML document
  xml_document<> doc;
  xml_node<>* decl = doc.allocate_node(node_declaration);
  decl->append_attribute(doc.allocate_attribute("version", "1.0"));
  decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
  doc.append_node(decl);
  xml_node<> *rootNode = doc.allocate_node(node_element, "root");
  doc.append_node(rootNode);

  saveToNode(doc, *rootNode, parent_path, analysisName);

  os << doc;
}

void Parameter::saveToFile(const boost::filesystem::path& file, std::string analysisName ) const
{
  CurrentExceptionContext ex("writing parameter set to file "+file.string());
  std::ofstream f(file.c_str());
  saveToStream( f, file.parent_path(), analysisName );
  f << std::endl;
  f << std::flush;
  f.close();
}


void Parameter::saveToString(std::string &s, const boost::filesystem::path& file, std::string analysisType) const
{
  std::ostringstream os(s, std::ios_base::ate);
  saveToStream(os, file.parent_path(), analysisType);
  s = os.str();
}



std::string Parameter::readFromRootNode(
    xml_node<>& rootNode,
    const boost::filesystem::path& parent_path,
    const std::string& startAtSubnode )
{
    CurrentExceptionContext ex("reading parameter %s from XML node", type().c_str());

    std::string analysisName;
    xml_node<> *analysisnamenode = rootNode.first_node("analysis");
    if (analysisnamenode)
    {
        analysisName = analysisnamenode->first_attribute("name")->value();
    }

    auto* crn=&rootNode;
    if (!startAtSubnode.empty())
    {
        std::vector<std::string> path;
        boost::split(path, startAtSubnode, boost::is_any_of("/"));
        for (const auto& p: path)
        {
            std::map<std::string, xml_node<>*> nodes;
            for (auto *e = crn->first_node(); e!=nullptr; e=e->next_sibling())
            {
                nodes[ e->first_attribute("name")->value() ]=e;
            }

            auto e = nodes.find(p);
            if (e==nodes.end())
            {
                std::ostringstream os;
                for(auto& n: nodes) os<<" "<<n.first;
                throw insight::Exception(
                    "Could not find node "+p+" (full path "+startAtSubnode+")!\n"
                                                                                   "Available:"+os.str());
            }
            else
            {
                crn=e->second;
            }
        }
    }

    readFromNode(std::string(), *crn, parent_path);

    return analysisName;
}




string Parameter::readFromFile(
    const boost::filesystem::path &file,
    const std::string &startAtSubnode )
{
    CurrentExceptionContext ex(
        "reading parameter %s from file %s",
        type().c_str(), file.string().c_str() );

    std::string contents;
    readFileIntoString(file, contents);

    xml_document<> doc;
    doc.parse<0>(&contents[0]);

    xml_node<> *rootnode = doc.first_node("root");

    return readFromRootNode(*rootnode, file.parent_path(), startAtSubnode);
}




bool Parameter::isPacked() const
{
  return false;
}

void Parameter::pack()
{
    // do nothing by default
}

void Parameter::unpack(const boost::filesystem::path&)
{
    // do nothing by default
}

void Parameter::clearPackedData()
{
    // do nothing by default
}

void Parameter::copyFrom(const Parameter &op)
{
  description_ = op.description_;
  isHidden_ = op.isHidden_;
  isExpert_ = op.isExpert_;
  isNecessary_ = op.isNecessary_;
  order_ = op.order_;

  if (!valueChangeSignalBlocked_) valueChanged();
}



void Parameter::extend ( const Parameter& op )
{
  // nothing to do, if there are no children
  insight::assertion(
      nChildren()==0,
      "internal error: extend function needs to be implemented!");

}


void Parameter::merge ( const Parameter& other )
{
  insight::assertion(
      nChildren()==0,
      "internal error: extend function needs to be implemented!");
  try
  {
    copyFrom(other);
  }
  catch (...)
  {
    // ignore, if not matching
  }
}


std::unique_ptr<Parameter> Parameter::intersection(const Parameter &other) const
{
  return std::unique_ptr<Parameter>(clone());
}


std::string Parameter::childParameterName( int i ) const
{
  if (nChildren()!=0)
    throw insight::Exception("internal error: childParameterRef() not implemented!");
  return std::string();
}


Parameter &Parameter::childParameterRef(int i)
{
  if (nChildren()!=0)
    throw insight::Exception("internal error: childParameterRef() not implemented!");
  return *this;
}

const Parameter &Parameter::childParameter(int i) const
{
  if (nChildren()!=0)
    throw insight::Exception("internal error: childParameter() not implemented!");
  return *this;
}


int Parameter::childParameterIndex( const std::string& name ) const
{
  for (int k=0; k<nChildren(); ++k)
  {
    if (childParameterName(k)==name) return k;
  }
  return -1;
}


Parameter& Parameter::childParameterByNameRef ( const std::string& name )
{
  int i=childParameterIndex(name);
  insight::assertion(
      i!=-1,
      "no parameter with name %s", name.c_str() );
  return childParameterRef(i);
}


const Parameter& Parameter::childParameterByName ( const std::string& name ) const
{
  int i=childParameterIndex(name);
  insight::assertion(
      i!=-1,
      "no parameter with name %s", name.c_str() );
  return childParameter(i);
}


Parameter::iterator Parameter::begin()
{
  return iterator(*this, 0);
}

Parameter::const_iterator Parameter::begin() const
{
  return cbegin();
}

Parameter::const_iterator Parameter::cbegin() const
{
  return const_iterator(*this, 0);
}

Parameter::iterator Parameter::end()
{
  return iterator(*this, nChildren());
}

Parameter::const_iterator Parameter::end() const
{
  return cend();
}

Parameter::const_iterator Parameter::cend() const
{
  return const_iterator(*this, nChildren());
}




void Parameter::setUpdateValueSignalBlockage(bool block)
{
  valueChangeSignalBlocked_=block;

  for (int i=0; i<nChildren(); ++i)
  {
    childParameterRef(i).setUpdateValueSignalBlockage(block);
  }
}




std::string valueToString(const arma::mat& value)
{
  std::string s;
  for (arma::uword i=0; i<value.n_elem; i++)
  {
    if (i>0) s+=" ";
    s += boost::str(boost::format("%g") % value(i));
  }
  return s;
}




void stringToValue(const std::string& s, arma::mat& v)
{
  CurrentExceptionContext ex("converting string \""+s+"\" into vector", false);

  std::vector<std::string> cmpts;
  auto st = boost::trim_copy(s);
  boost::split(
        cmpts,
        st,
        boost::is_any_of(" \t\n,;"),
        token_compress_on
        );
  std::vector<double> vals;
  for (size_t i=0; i<cmpts.size(); i++)
  {
    vals.push_back( toNumber<double>(cmpts[i]) );
  }

  v=arma::mat(vals.data(), vals.size(), 1);
}




 



}
