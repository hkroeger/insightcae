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
: boost::noncopyable
{
public:
  typedef boost::tuple<const std::string&, const std::string&, const std::string&> ResultElementConstrP;
  declareFactoryTable(ResultElement, ResultElementConstrP); 
  
protected:
  std::string shortDescription_;
  std::string longDescription_;
  std::string unit_;
  
public:
  declareType("ResultElement");
  
  ResultElement(const ResultElementConstrP& par);
  virtual ~ResultElement();
  
  inline const std::string& shortDescription() const { return shortDescription_; }
  inline const std::string& longDescription() const { return longDescription_; }
  inline const std::string& unit() const { return unit_; }
  
  virtual void writeLatexHeaderCode(std::ostream& f) const;
  virtual void writeLatexCode(std::ostream& f, int level) const;
  
  virtual ResultElement* clone() const =0;
};

typedef std::auto_ptr<ResultElement> ResultElementPtr;


class Image
: public ResultElement
{
protected:
  boost::filesystem::path imagePath_;
public:
  declareType("Image");
  Image(const ResultElementConstrP& par);
  Image(const boost::filesystem::path& value, const std::string& shortDesc, const std::string& longDesc);
  
  inline const boost::filesystem::path& imagePath() const { return imagePath_; }
  inline void setPath(const boost::filesystem::path& value) { imagePath_=value; }
  
  virtual void writeLatexHeaderCode(std::ostream& f) const;
  virtual void writeLatexCode(std::ostream& f, int level) const;
  virtual ResultElement* clone() const;
};


template<class T>
class NumericalResult
: public ResultElement
{
protected:
  T value_;
  
public:
  
  NumericalResult(const ResultElementConstrP& par)
  : ResultElement(par)
  {}

  NumericalResult(const T& value, const std::string& shortDesc, const std::string& longDesc, const std::string& unit)
  : ResultElement(ResultElementConstrP(shortDesc, longDesc, unit)),
    value_(value)
  {}
  
  inline void setValue(const T& value) { value_=value; }
  
  inline const T& value() const { return value_; }
};

class ScalarResult
: public NumericalResult<double>
{
public:
  declareType("ScalarResult");
  
  ScalarResult(const ResultElementConstrP& par);
  ScalarResult(const double& value, const std::string& shortDesc, const std::string& longDesc, const std::string& unit);
  virtual void writeLatexCode(std::ostream& f, int level) const;
  virtual ResultElement* clone() const;
};


class TabularResult
: public ResultElement
{
public:
  typedef std::vector<std::vector<double> > Table;
  
protected:
  std::vector<std::string> headings_;
  Table rows_;
  
public:
  declareType("TabularResult");
  
  TabularResult(const ResultElementConstrP& par);
  
  TabularResult
  (
   const std::vector<std::string>& headings, 
   const Table& rows, 
   const std::string& shortDesc, 
   const std::string& longDesc,
   const std::string& unit
  );
  
  inline const std::vector<std::string>& headings() const { return headings_; }
  inline const Table& rows() const { return rows_; }
  
  inline void setTableData(const std::vector<std::string>& headings, const Table& rows) { headings_=headings; rows_=rows; }
  
  virtual void writeGnuplotData(std::ostream& f) const;
  virtual void writeLatexCode(std::ostream& f, int level) const;
  
  virtual ResultElement* clone() const;
};


class ResultSet
: public boost::ptr_map<std::string, ResultElement>,
  public ResultElement
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
    const std::string *author = NULL,
    const std::string *date = NULL
  );
  
  ResultSet(const ResultSet& other);
  virtual ~ResultSet();
  
  inline const std::string& title() const { return title_; }
  inline const std::string& subtitle() const { return subtitle_; }
  
  void transfer(const ResultSet& other);
  inline const ParameterSet& parameters() const { return p_; }
  
  virtual void writeLatexHeaderCode(std::ostream& f) const;
  virtual void writeLatexCode(std::ostream& f, int level) const;

  virtual void writeLatexFile(const boost::filesystem::path& file) const;
  
  virtual ResultElement* clone() const;
};

typedef boost::shared_ptr<ResultSet> ResultSetPtr;

inline ResultElement* new_clone(const ResultElement& e)
{
  return e.clone();
}

}



#endif // INSIGHT_RESULTSET_H
