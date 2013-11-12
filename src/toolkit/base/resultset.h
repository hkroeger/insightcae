/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  Hannes Kroeger <email>

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


#ifndef INSIGHT_RESULTSET_H
#define INSIGHT_RESULTSET_H

#include "base/parameterset.h"

#include <string>

#include "boost/ptr_container/ptr_map.hpp"
#include "boost/filesystem.hpp"
#include "boost/gil/gil_all.hpp"

namespace insight 
{
  
class ResultElement
{
protected:
  std::string shortDescription_;
  std::string longDescription_;
  std::string unit_;
  
public:
  ResultElement(const std::string& shortDesc, const std::string& longDesc, const std::string& unit);
  virtual ~ResultElement();
  
  inline const std::string& shortDescription() const { return shortDescription_; }
  inline const std::string& longDescription() const { return longDescription_; }
  inline const std::string& unit() const { return unit_; }
  
  virtual void writeLatexHeaderCode(std::ostream& f) const;
  virtual void writeLatexCode(std::ostream& f) const;
};

/*
class Image
: public ResultElement
{
protected:
};
*/

template<class T>
class NumericalResult
: public ResultElement
{
protected:
  T value_;
  
public:
  NumericalResult(const T& value, const std::string& shortDesc, const std::string& longDesc, const std::string& unit)
  : ResultElement(shortDesc, longDesc, unit),
    value_(value)
  {}
};

class ScalarResult
: public NumericalResult<double>
{
public:
  ScalarResult(const double& value, const std::string& shortDesc, const std::string& longDesc, const std::string& unit);
  virtual void writeLatexCode(std::ostream& f) const;
};


class ResultSet
: public boost::ptr_map<std::string, ResultElement>
{
protected:
  ParameterSet p_;
  std::string title_, subtitle_, date_, author_;
  
public:
  ResultSet
  (
    const ParameterSet& p,
    const std::string& title,
    const std::string& subtitle,
    std::string *author = NULL,
    std::string *date = NULL
  );
  virtual ~ResultSet();
  
  virtual void writeLatexFile(const boost::filesystem::path& file) const;
};

typedef boost::shared_ptr<ResultSet> ResultSetPtr;

}

#endif // INSIGHT_RESULTSET_H
