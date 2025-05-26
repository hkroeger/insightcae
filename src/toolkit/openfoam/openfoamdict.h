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
#include <typeinfo>
#include <cxxabi.h>

#include "base/boost_include.h"

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
    
double as_scalar(const data& d);
    
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

  list(std::initializer_list<OFDictData::data> ini);

  template<class T>
  list(const std::vector<T>& ol)
  {
      std::copy(ol.begin(), ol.end(), std::back_inserter(*this));
  }

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
    {
        std::string keys=" ";
        for (const value_type& it: *this) { keys+=it.first+" "; }
        throw Exception("key "+key+" not found! Available keys:"+keys);
    }
    if (T *d = boost::get<T>(&i->second))
      return *d;
    else
      throw Exception("entry "+key+" is there but not of the requested type!"
                 " (actual type:"+boost::lexical_cast<std::string>(i->second.which())+")"
//               "(requested: "+std::string(typeid(T)::name())+", actual: "+std::string(typeid(i->second)::name())+")" 
                );
  }
  
  template<class T>
  const T& lookup(const std::string& key) const
  {
    dict::const_iterator i=this->find(key);
    if (i==this->end())
    {
        std::string keys=" ";
        for (const value_type& it: *this) { keys+=it.first+" "; }
        throw Exception("key "+key+" not found! Available: "+keys);
    }
    if (const T *d = boost::get<T>(&i->second))
      return *d;
    else
      throw Exception("entry "+key+" is there but not of the requested type!"
                 " (actual type:"+boost::lexical_cast<std::string>(i->second.which())+")"
                );
  }
  
  /**
   * lookup subdict
   * add subdict, if requested and no such key exists
   * return existing subdict only otherwise
   * raise exception, if key exists and is not a subdict
   */
  dict& subDict(const std::string& key, bool createIfNonexistent=true);

  inline const dict& subDict(const std::string& key) const
  {
    return this->lookup<dict>(key);
  }

  list& getList(const std::string& key, bool createIfNonexistent=true);

  inline double& getDoubleRef(const std::string& key)
  {
    return this->lookup<double>(key);
  }

  inline double getDouble(const std::string& key) const
  {
     return this->getDoubleOrInt(key);
  }

  inline double getDoubleOrInt(const std::string& key) const
  {
     auto i = this->find(key);
     if (i==this->end())
     {
      std::string keys=" ";
      for (const value_type& it: *this) { keys+=it.first+" "; }
      throw Exception("key "+key+" not found! Available keys:"+keys);
     }
     if (auto v = boost::get<double>(&i->second))
        return *v;
     else if (auto v = boost::get<int>(&i->second))
      return *v;
     else
      throw Exception("entry "+key+" is there but not of the requested type!"
                      " (actual type:"+boost::lexical_cast<std::string>(i->second.which())+")"
                      );
  }

  inline int& getIntRef(const std::string& key)
  {
    return this->lookup<int>(key);
  }

  inline int getInt(const std::string& key) const
  {
    return this->lookup<int>(key);
  }

  inline std::string& getStringRef(const std::string& key)
  {
    return this->lookup<std::string>(key);
  }

  inline std::string getString(const std::string& key) const
  {
    return this->lookup<std::string>(key);
  }
  
  void write(std::ostream& os, int indentLevel=0) const;

  std::vector<std::string> findKeys(const boost::regex& re) const;

};

struct dictFile
: public dict
{
  std::string className;
  int dictVersion;
  int OFversion;
  bool isSequential;
  bool no_header;
  
  dictFile();
  
  void write(const boost::filesystem::path& dictPath) const;
};


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


std::string toString(const data& d);
std::string toUniformField(const data& d);
std::string toUniformField(const arma::mat& v);

}



/**
 * reads OF dict
 * filename without possible ".gz". Will detect compressed dict and uncompress in temp dir
 */
void readOpenFOAMDict(const boost::filesystem::path& dictFile, OFDictData::dict& d);

bool readOpenFOAMDict(std::istream& in, OFDictData::dict& d);
void writeOpenFOAMDict(std::ostream& out, const OFDictData::dictFile& d, const std::string& objname);
void writeOpenFOAMDict(const boost::filesystem::path& dictpath, const OFDictData::dictFile& dict);

bool readOpenFOAMBoundaryDict(std::istream& in, OFDictData::dict& d);
void writeOpenFOAMBoundaryDict(std::ostream& out, const OFDictData::dictFile& d, bool filterZeroSizesPatches=false);
bool patchExists(const OFDictData::dict& bd, const std::string& patchName);

void writeOpenFOAMSequentialDict(std::ostream& out, const OFDictData::dictFile& d, const std::string& objname, bool skip_header=false);


}

#endif // INSIGHT_OPENFOAMDICT_H
