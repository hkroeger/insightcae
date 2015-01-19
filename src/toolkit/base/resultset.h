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


#ifndef INSIGHT_RESULTSET_H
#define INSIGHT_RESULTSET_H

#include "base/parameterset.h"
#include "base/linearalgebra.h"

#include <string>
#include <vector>

#include "base/boost_include.h"
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
  
  ResultElement(const ResultElement::ResultElementConstrP& par);
  virtual ~ResultElement();
  
  inline const std::string& shortDescription() const { return shortDescription_; }
  inline const std::string& longDescription() const { return longDescription_; }
  inline const std::string& unit() const { return unit_; }
  
  virtual void writeLatexHeaderCode(std::ostream& f) const;
  virtual void writeLatexCode(std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const;
  virtual void exportDataToFile(const std::string& name, const boost::filesystem::path& outputdirectory) const;
  
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
  Image(const ResultElement::ResultElementConstrP& par);
  Image(const boost::filesystem::path& location, const boost::filesystem::path& value, const std::string& shortDesc, const std::string& longDesc);
  
  inline const boost::filesystem::path& imagePath() const { return imagePath_; }
  inline void setPath(const boost::filesystem::path& value) { imagePath_=value; }
  
  virtual void writeLatexHeaderCode(std::ostream& f) const;
  virtual void writeLatexCode(std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const;
  virtual ResultElement* clone() const;
};


template<class T>
class NumericalResult
: public ResultElement
{
protected:
  T value_;
  
public:
  
  NumericalResult(const ResultElement::ResultElementConstrP& par)
  : ResultElement(par)
  {}

  NumericalResult(const T& value, const std::string& shortDesc, const std::string& longDesc, const std::string& unit)
  : ResultElement(ResultElement::ResultElementConstrP(shortDesc, longDesc, unit)),
    value_(value)
  {}
  
  inline void setValue(const T& value) { value_=value; }
  
  inline const T& value() const { return value_; }
  
  virtual void exportDataToFile(const std::string& name, const boost::filesystem::path& outputdirectory) const
  {
    boost::filesystem::path fname(outputdirectory/(name+".dat"));
    std::ofstream f(fname.c_str());
    f<<value_<<std::endl;
  }
};

class Comment
: public ResultElement
{
protected:
  std::string value_;
  
public:
  declareType("Comment");
  
  Comment(const ResultElement::ResultElementConstrP& par);
  Comment(const std::string& value, const std::string& shortDesc, const std::string& longDesc);
  virtual void writeLatexCode(std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const;
  virtual ResultElement* clone() const;
};

class ScalarResult
: public NumericalResult<double>
{
public:
  declareType("ScalarResult");
  
  ScalarResult(const ResultElement::ResultElementConstrP& par);
  ScalarResult(const double& value, const std::string& shortDesc, const std::string& longDesc, const std::string& unit);
  virtual void writeLatexCode(std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const;
  virtual ResultElement* clone() const;
};


class TabularResult
: public ResultElement
{
public:
  typedef std::vector<double> Row;
  typedef std::vector<Row> Table;
  
protected:
  std::vector<std::string> headings_;
  Table rows_;
  
public:
  declareType("TabularResult");
  
  TabularResult(const ResultElement::ResultElementConstrP& par);
  
  TabularResult
  (
   const std::vector<std::string>& headings, 
   const Table& rows, 
   const std::string& shortDesc, 
   const std::string& longDesc,
   const std::string& unit
  );
  
  TabularResult
  (
   const std::vector<std::string>& headings, 
   const arma::mat& rows, 
   const std::string& shortDesc, 
   const std::string& longDesc,
   const std::string& unit
  );
  
  inline const std::vector<std::string>& headings() const { return headings_; }
  inline const Table& rows() const { return rows_; }
  inline Row& appendRow() 
  { 
    rows_.push_back(Row(headings_.size()));
    return rows_.back();
  }
  void setCellByName(Row& r, const std::string& colname, double value);
  
  arma::mat getColByName(const std::string& colname) const;
  
  arma::mat toMat() const;
  
  inline void setTableData(const std::vector<std::string>& headings, const Table& rows) { headings_=headings; rows_=rows; }
  
  virtual void writeGnuplotData(std::ostream& f) const;
  virtual void writeLatexHeaderCode(std::ostream& f) const;
  virtual void writeLatexCode(std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const;
  
  virtual ResultElement* clone() const;
};


class AttributeTableResult
: public ResultElement
{
public:
  typedef std::vector<std::string> AttributeNames;
  typedef boost::variant<int, double, std::string> AttributeValue;
  typedef std::vector<AttributeValue> AttributeValues;
  
protected:
  AttributeNames names_;
  AttributeValues values_;
  
public:
  declareType("AttributeTableResult");
  
  AttributeTableResult(const ResultElement::ResultElementConstrP& par);
  
  AttributeTableResult
  (
    AttributeNames names,
    AttributeValues values, 
   const std::string& shortDesc, 
   const std::string& longDesc,
   const std::string& unit
  );
  
  inline void setTableData(AttributeNames names, AttributeValues values) 
  { names_=names; values_=values; }
  
  inline const AttributeNames& names() const { return names_; }
  inline const AttributeValues& values() const { return values_; }
  
  virtual void writeLatexCode(std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const;
  
  virtual ResultElement* clone() const;
};

ResultElementPtr polynomialFitResult
(
  const arma::mat& coeffs, 
  const std::string& xvarName,
  const std::string& shortDesc, 
  const std::string& longDesc,
  int minorder=0
);

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
  virtual void writeLatexCode(std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const;

  virtual void exportDataToFile(const std::string& name, const boost::filesystem::path& outputdirectory) const;
  virtual void writeLatexFile(const boost::filesystem::path& file) const;
  
  template<class T>
  inline T& get(const std::string& name)
  {
    ResultSet::iterator i=find(name);
    if (i==end())
      insight::Exception("ResultSet does not contain element "+name);
    T* ret=dynamic_cast<T*>(i->second);
    if (!ret)
      insight::Exception("ResultSet does contain element "+name+" but it is not of requested type "+T::typeName);
    return *ret;
  }
  
  virtual ResultElement* clone() const;
};

typedef boost::shared_ptr<ResultSet> ResultSetPtr;

inline ResultElement* new_clone(const ResultElement& e)
{
  return e.clone();
}


struct PlotCurve
{
  arma::mat xy_;
  std::string plotcmd_;
  
  PlotCurve();
  PlotCurve(const char* plotcmd);
  PlotCurve(const std::vector<double>& x, const std::vector<double>& y, const std::string& plotcmd = "");
  PlotCurve(const arma::mat& xy, const std::string& plotcmd = "w l");
  
  std::string title() const;
};

typedef std::vector<PlotCurve> PlotCurveList;

void addPlot
(
  ResultSetPtr& results,
  const boost::filesystem::path& workdir, 
  const std::string& resultelementname,
  const std::string& xlabel,
  const std::string& ylabel,
  const PlotCurveList& plc,
  const std::string& shortDescription,
  const std::string& addinit = ""
);

class Chart
: public ResultElement
{
protected:
  std::string xlabel_;
  std::string ylabel_;
  PlotCurveList plc_;
  std::string addinit_;
  
public:
  declareType("Chart");
  Chart(const ResultElement::ResultElementConstrP& par);
  Chart
  (
    const std::string& xlabel,
    const std::string& ylabel,
    const PlotCurveList& plc,
    const std::string& shortDesc, const std::string& longDesc,
    const std::string& addinit = ""    
  );
  
  virtual void generatePlotImage(const boost::filesystem::path& imagepath) const;
  
  virtual void writeLatexHeaderCode(std::ostream& f) const;
  virtual void writeLatexCode(std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath) const;
  virtual void exportDataToFile(const std::string& name, const boost::filesystem::path& outputdirectory) const;
  
  virtual ResultElement* clone() const;
};

struct PlotField
{
  arma::mat xy_;
  std::string plotcmd_;
  
  PlotField();
  PlotField(const arma::mat& xy, const std::string& plotcmd = "");
};

typedef std::vector<PlotCurve> PlotFieldList;

void addContourPlot
(
  ResultSetPtr& results,
  const boost::filesystem::path& workdir, 
  const std::string& resultelementname,
  const std::string& xlabel,
  const std::string& ylabel,
  const PlotFieldList& plc,
  const std::string& shortDescription,
  const std::string& addinit = ""
);

}



#endif // INSIGHT_RESULTSET_H
