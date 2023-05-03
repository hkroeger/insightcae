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



using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace rapidxml;



namespace insight 
{
  
    
rapidxml::xml_node<> *findNode(rapidxml::xml_node<>& father, const std::string& name, const std::string& typeName)
{
  for (xml_node<> *child = father.first_node(typeName.c_str()); child; child = child->next_sibling(typeName.c_str()))
  {
    if (child->first_attribute("name")->value() == name)
    {
      return child;
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

Parameter::Parameter()
{
}

Parameter::Parameter(const std::string& description, bool isHidden, bool isExpert, bool isNecessary, int order)
: description_(description),
  isHidden_(isHidden), isExpert_(isExpert), isNecessary_(isNecessary), order_(order)
{
}

Parameter::~Parameter()
{
}


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
    xml_node<>* child = doc.allocate_node(node_element, doc.allocate_string(this->type().c_str()));
    node.append_node(child);
    child->append_attribute(doc.allocate_attribute
    (
      "name", 
      doc.allocate_string(name.c_str()))
    );

    return child;
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

void Parameter::reset(const Parameter& op)
{
  description_ = op.description_;
  isHidden_ = op.isHidden_;
  isExpert_ = op.isExpert_;
  isNecessary_ = op.isNecessary_;
  order_ = op.order_;
}



Parameter::SearchParentResult
Parameter::searchMyParentIn(const ParameterSet &ps) const
{
  SearchParentResult result = boost::blank();

  std::function<void(const ParameterSet *, const Parameter* p)> searchSubdicts;
  std::function<void(const ArrayParameterBase *)> searchArray;
  std::function<void(const Parameter* p)> searchParameter;

  searchParameter = [&](const Parameter* p)
  {
    if (const auto* sps = dynamic_cast<const SubParameterSet*>(p))
    {
      searchSubdicts( &(sps->subset()), p );
    }
    else if (const auto* sa = dynamic_cast<const ArrayParameterBase*>(p))
    {
      searchArray( sa );
    }
  };

  searchSubdicts = [&](const ParameterSet *ps, const Parameter* p)
  {
    for (auto entry=ps->begin(); entry!=ps->end(); ++entry)
    {
      if (this == entry->second.get())
      {
        SearchResultParentInDict r;
        r.myIterator=entry;
        r.myParentSet=ps;
        r.myParentSetParameter=p;
        result=r;
        break;
      }
      searchParameter(entry->second.get());
      if ( !boost::get<boost::blank>(&result) ) break;
    }
  };

  searchArray = [&](const ArrayParameterBase *a)
  {
    for (int i=0; i<a->size(); ++i)
    {
      if ( this == &a->element(i) )
      {
        SearchResultParentInArray r;
        r.i=i;
        r.myParentArrayParameter=dynamic_cast<const Parameter*>(a);
        result=r;
        break;
      }
      searchParameter( &a->element(i) );
      if ( !boost::get<boost::blank>(&result) ) break;
    }
  };

  searchSubdicts(&ps, nullptr);

  insight::assertion(
        !boost::get<boost::blank>(&result),
        "No parent section found for this parameter in supplied parameter set!" );

  return result;
}


std::string valueToString(const arma::mat& value)
{
  std::string s;
  for (arma::uword i=0; i<value.n_elem; i++)
  {
    if (i>0) s+=" ";
    s += boost::str(boost::format("%g") % value(i)); //boost::lexical_cast<string>(value(i));
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
