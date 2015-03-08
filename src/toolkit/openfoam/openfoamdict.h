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


#ifndef INSIGHT_OPENFOAMDICT_H
#define INSIGHT_OPENFOAMDICT_H

#include <fstream>
#include <iostream>
#include <stack>
#include <functional>
#include <string>

#include "base/boost_include.h"
// #include "boost/spirit/include/qi.hpp"
// #include "boost/spirit/repository/include/qi_confix.hpp"
// #include <boost/spirit/include/qi_eol.hpp>

#include "base/linearalgebra.h"
#include "base/exception.h"

namespace insight
{
 

namespace OFDictData
{
  
  const char SPACE[]=" ";
  
typedef std::vector<int> dimensionSet;
dimensionSet dimension(int d0=0, int d1=0, int d2=0, int d3=0, int d4=0, int d5=0, int d6=0);
  
struct dict;
struct list;
struct dimensionedData;

typedef boost::variant<
    std::string,
    double,
    int,
    boost::recursive_wrapper<dimensionedData>,
    boost::recursive_wrapper<dict>,
    boost::recursive_wrapper<list>
    > data;
    
typedef std::pair<std::string, data> entry;

struct dimensionedData
: public boost::fusion::tuple<
  std::string,
  std::vector<int>,
  data>
{
    dimensionedData();
    //dimensionedData(const std::string&, const std::vector<int>&, const data&);
    dimensionedData(const std::string&, const std::vector<int>&, const data&);
};

struct list 
: public std::vector<data> 
{
  list();
  
  template<class T>
  void assign(const std::vector<T>& vec)
  {
    resize(vec.size());
    std::copy(vec.begin(), vec.end(), begin());
  }
  
  void insertNoDuplicate(const OFDictData::data& d);
};

struct dict 
: public std::map<std::string, data> 
{
  
  template<class T>
  T& lookup(const std::string& key)
  {
    dict::iterator i=this->find(key);
    if (i==this->end())
      throw Exception("key "+key+" not found!");
    if (T *d = boost::get<T>(&i->second))
      return *d;
    else
      throw Exception("entry "+key+" is there but not of the requested type!");
  }
  
  template<class T>
  const T& lookup(const std::string& key) const
  {
    dict::const_iterator i=this->find(key);
    if (i==this->end())
      throw Exception("key "+key+" not found!");
    if (const T *d = boost::get<T>(&i->second))
      return *d;
    else
      throw Exception("entry "+key+" is there but not of the requested type!");
  }
  
  /**
   * add subdict, if no such key exists, return existing subdict otherwise
   * raise exception, if key exists and is not a subdict
   */
  dict& addSubDictIfNonexistent(const std::string& key);
  list& addListIfNonexistent(const std::string& key);
  
  inline dict& subDict(const std::string& key)
  {
    return this->lookup<dict>(key);
  }

  inline const dict& subDict(const std::string& key) const
  {
    return this->lookup<dict>(key);
  }

  inline list& getList(const std::string& key)
  {
    return this->lookup<list>(key);
  }

  inline double& getDouble(const std::string& key)
  {
    return this->lookup<double>(key);
  }

  inline const double& getDouble(const std::string& key) const
  {
    return this->lookup<double>(key);
  }

  inline int& getInt(const std::string& key)
  {
    return this->lookup<int>(key);
  }

  inline const int& getInt(const std::string& key) const
  {
    return this->lookup<int>(key);
  }

  inline std::string& getString(const std::string& key)
  {
    return this->lookup<std::string>(key);
  }

  inline const std::string& getString(const std::string& key) const
  {
    return this->lookup<std::string>(key);
  }
  
  void write(std::ostream& os, int indentLevel=0) const;

};

struct dictFile
: public dict
{
  std::string className;
  int dictVersion;
  int OFversion;
  bool isSequential;
  
  dictFile();
  
  void write(const boost::filesystem::path& dictPath) const;
};

std::string to_OF(const arma::mat& v);

/**
 * Return the first three elements of given vector as OFDictData::list
 * @v: vector data. Only the first three elements are used
 */
OFDictData::list vector3(const arma::mat& v);

/**
 * Return the elements of given vector/matrix as OFDictData::data
 * @v: vector/tensor data. All elements are concatened and a list is returned for more than one element. A double entry is returned for a 1x1 matrix.
 */
OFDictData::data vectorSpace(const arma::mat& v);

/**
 * Return a vector as OFDictData::list
 * @x: x-component
 * @y: y-component
 * @z: z-component
 */
OFDictData::list vector3(double x, double y, double z);

std::ostream& operator<<(std::ostream& os, const dimensionSet& d);
std::ostream& operator<<(std::ostream& os, const dimensionedData& d);
std::ostream& operator<<(std::ostream& os, const dict& d);
std::ostream& operator<<(std::ostream& os, const list& l);

}



void readOpenFOAMDict(std::istream& in, OFDictData::dict& d);
void writeOpenFOAMDict(std::ostream& out, const OFDictData::dictFile& d, const std::string& objname);

void readOpenFOAMBoundaryDict(std::istream& in, OFDictData::dict& d);
void writeOpenFOAMBoundaryDict(std::ostream& out, const OFDictData::dictFile& d);

void writeOpenFOAMSequentialDict(std::ostream& out, const OFDictData::dictFile& d, const std::string& objname);

bool patchExists(const OFDictData::dict& bd, const std::string& patchName);

}

#endif // INSIGHT_OPENFOAMDICT_H
