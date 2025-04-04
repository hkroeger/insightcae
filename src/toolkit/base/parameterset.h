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


#ifndef INSIGHT_PARAMETERSET_H
#define INSIGHT_PARAMETERSET_H

#include <memory>
#include <map>
#include <vector>
#include <iostream>
#include <algorithm>

#include "base/exception.h"
#include "base/parameter.h"
#include "base/parameters/arrayparameter.h"
#include "base/parameters/subsetparameter.h"
#include "base/progressdisplayer.h"


#include "boost/tuple/tuple.hpp"
#include "boost/fusion/tuple.hpp"
#include "boost/algorithm/string.hpp"

#include "rapidxml/rapidxml.hpp"


class QoccViewWidget;
class QModelTree;
class QIcon;


namespace insight {
  



//class SubsetParameter;
class SelectableSubsetParameter;




///**
// * @brief The SubParameterSet class
// * common base class for parameters that contain a set of parameters
// */
//class SubParameterSet
//{
//public:
//  virtual ~SubParameterSet();

//  ParameterSet& subsetRef();
//  virtual const ParameterSet& subset() const =0;

//  virtual void merge(const SubParameterSet& other, bool allowInsertion) =0;
//  virtual Parameter* intersection(const SubParameterSet& other) const =0;
//};






// class ParameterSet
//     : public SubsetParameter
// {

// public:
//     ParameterSet();

//     ParameterSet(
//         SubsetParameter::Entries &&defaultValue,
//         const std::string& description = std::string() );

//     ParameterSet(
//         const SubsetParameter::EntryReferences &defaultValue,
//         const std::string& description = std::string() );

//     virtual ~ParameterSet();

//     void copyFrom(const Parameter& o) override;
//     void operator=(const ParameterSet& o);

//     /**
//    * insert values from other, where matching. Ignore non-matching parameters.
//    * return a non-const reference to this PS to anable call chains like PS.merge().merge()...
//    */
//     ParameterSet& merge ( const ParameterSet& other );

//     /**
//    * @brief intersection
//    * construct a ParameterSet which contains only elements
//    * that are present both in this set and "other"
//    * @param other
//    * @return a new ParameterSet with the common elements
//    */
//     std::unique_ptr<ParameterSet>
//     intersection(const ParameterSet& other) const;

//     virtual std::unique_ptr<ParameterSet> cloneParameterSet() const;

//     virtual void appendToNode ( rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
//                               boost::filesystem::path inputfilepath ) const;
//     virtual void readFromNode ( rapidxml::xml_node<>& node,
//                               boost::filesystem::path inputfilepath );


// };










class ParameterSet_Validator
{
public:
    typedef std::map<std::string, std::string> WarningList;
    typedef std::map<std::string, std::string> ErrorList;

protected:
    ParameterSet* ps_;


    WarningList warnings_;
    ErrorList errors_;

public:
    virtual ~ParameterSet_Validator();

    /**
     * @brief update
     * @param ps
     * checks a parameter set (needs to be customized by derivation for that)
     * Stores a copy of the parameter set and updates the warnings and errors list.
     * Needs to be called first in derived classes.
     */
    virtual void update(const ParameterSet& ps);

    virtual bool isValid() const;
    virtual const WarningList& warnings() const;
    virtual const ErrorList& errors() const;
};


typedef std::shared_ptr<ParameterSet_Validator> ParameterSet_ValidatorPtr;








//template<class T>
//T& ParameterSet::get ( const std::string& name )
//{
//  typedef T PT;

//  auto& p = this->getParameter(name);

//  if ( PT* const pt=dynamic_cast<PT* const>(&p) )
//  {
//    return *pt;
//  }
//  else
//  {
//    throw insight::Exception(
//          "Parameter "+name+" not of requested type!"
//          " (actual type is "+p.type()+")"
//          );
//  }
//}

}


#endif // INSIGHT_PARAMETERSET_H
