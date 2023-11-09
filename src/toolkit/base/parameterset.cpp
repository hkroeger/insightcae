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





ParameterSet::ParameterSet()
{
}

ParameterSet::ParameterSet(const ParameterSet &o)
{
    operator=(o);
}


ParameterSet::ParameterSet ( const SubsetParameter& o )
{
    SubsetParameter::operator=(o);
}


ParameterSet::ParameterSet(const EntryCopies &defaultValue, const std::string &description)
    : SubsetParameter(defaultValue, description)
{}


ParameterSet::ParameterSet(const EntryReferences &defaultValue, const std::string &description)
    : SubsetParameter(defaultValue, description)
{}


ParameterSet::~ParameterSet()
{
}



void ParameterSet::setParameterSetDescription(const std::string& desc)
{
  description_.simpleLatex() = desc;
}

const SimpleLatex& ParameterSet::parameterSetDescription() const
{
  return description_;
}


void ParameterSet::copyFrom(const Parameter& o)
{
  operator=(dynamic_cast<const ParameterSet&>(o));
}


void ParameterSet::operator=(const ParameterSet &o)
{
  SubsetParameter::operator=(o);
}




ParameterSet& ParameterSet::merge(const ParameterSet& other )
{
  SubsetParameter::merge(other);
  return *this;
}



ParameterSet ParameterSet::intersection(const ParameterSet &other) const
{
  ParameterSet ps;
  ps.SubsetParameter::copyFrom( *SubsetParameter::intersection(other) );
  return ps;
}




ParameterSet* ParameterSet::cloneParameterSet() const
{
  ParameterSet *np=new ParameterSet;
  np->SubsetParameter::copyFrom( *this );
  return np;
}

void ParameterSet::appendToNode(rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath) const
{
  SubsetParameter::appendToNode(std::string(), doc, node, inputfilepath);
}

void ParameterSet::readFromNode(
    rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath)
{
  SubsetParameter::readFromNode(std::string(), node, inputfilepath);
}


void ParameterSet::packExternalFiles()
{
  SubsetParameter::pack();
}

void ParameterSet::removePackedData()
{
  SubsetParameter::clearPackedData();
}

void ParameterSet::unpackAllExternalFiles(const boost::filesystem::path& basePath)
{
  SubsetParameter::unpack(basePath);
}



std::ostream& operator<<(std::ostream& os, const ParameterSet& ps)
{
  CurrentExceptionContext ex(2, "writing plain text representation of parameter set to output stream (via << operator)");
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

