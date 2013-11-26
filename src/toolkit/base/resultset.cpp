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


#include "resultset.h"

#include <fstream>

//#include "boost/gil/gil_all.hpp"
//#include "boost/gil/extension/io/jpeg_io.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/foreach.hpp"
#include "boost/date_time.hpp"

using namespace std;
using namespace boost;


namespace insight
{

defineType(ResultElement);
defineFactoryTable(ResultElement, ResultElement::ResultElementConstrP);

ResultElement::ResultElement(const ResultElement::ResultElementConstrP& par)
: shortDescription_(boost::get<0>(par)),
  longDescription_(boost::get<1>(par)),
  unit_(boost::get<2>(par))
{}

ResultElement::~ResultElement()
{}


void ResultElement::writeLatexHeaderCode(std::ostream& f) const
{
}

void ResultElement::writeLatexCode(ostream& f) const
{
}

defineType(Image);
addToFactoryTable(ResultElement, Image, ResultElement::ResultElementConstrP);


Image::Image(const ResultElementConstrP& par)
: ResultElement(par)
{
}

Image::Image(const boost::filesystem::path& value, const std::string& shortDesc, const std::string& longDesc)
: ResultElement(ResultElementConstrP(shortDesc, longDesc, "")),
  imagePath_(value)
{
}

void Image::writeLatexHeaderCode(std::ostream& f) const
{
  f<<"\\usepackage{graphicx}\n";
}

void Image::writeLatexCode(std::ostream& f) const
{
  f<< "\\includegraphics[keepaspectratio,width=\\textwidth]{" << imagePath_.c_str() << "}\n";
}

ResultElement* Image::clone() const
{
  return new Image(imagePath_, shortDescription_, longDescription_);
}
  

defineType(ScalarResult);
addToFactoryTable(ResultElement, ScalarResult, ResultElement::ResultElementConstrP);


ScalarResult::ScalarResult(const ResultElementConstrP& par)
: NumericalResult< double >(par)
{
}

ScalarResult::ScalarResult(const double& value, const string& shortDesc, const string& longDesc, const string& unit)
: NumericalResult< double >(value, shortDesc, longDesc, unit)
{}

void ScalarResult::writeLatexCode(ostream& f) const
{
  f.setf(ios::fixed,ios::floatfield);
  f.precision(3);
  f << value_ << unit_;
}

ResultElement* ScalarResult::clone() const
{
  return new ScalarResult(value_, shortDescription_, longDescription_, unit_);
}


defineType(TabularResult);
addToFactoryTable(ResultElement, TabularResult, ResultElement::ResultElementConstrP);


TabularResult::TabularResult(const ResultElementConstrP& par)
: ResultElement(par)
{
}

TabularResult::TabularResult
(
  const std::vector<std::string>& headings, 
  const Table& rows,
  const std::string& shortDesc, 
  const std::string& longDesc,
  const std::string& unit
)
: ResultElement(ResultElementConstrP(shortDesc, longDesc, unit))
{
  setTableData(headings, rows);
}

void TabularResult::writeGnuplotData(std::ostream& f) const
{
  f<<"#";
  BOOST_FOREACH(const std::string& head, headings_)
  {
     f<<" \""<<head<<"\"";
  }
  f<<std::endl;

  BOOST_FOREACH(const std::vector<double>& row, rows_)
  {
    BOOST_FOREACH(double v, row)
    {
      f<<" "<<v;
    }
    f<<std::endl;
  }

}

ResultElement* TabularResult::clone() const
{
  return new TabularResult(headings_, rows_, shortDescription_, longDescription_, unit_);
}


void TabularResult::writeLatexCode(std::ostream& f) const
{
  f<<
  "\\begin{tabular}{";
  BOOST_FOREACH(const std::string& h, headings_)
  {
    f<<"c";
  }
  f<<"}\n";
  for (std::vector<std::string>::const_iterator i=headings_.begin(); i!=headings_.end(); i++)
  {
    if (i!=headings_.begin()) f<<" & ";
    f<<*i;
  }
  f<<
  "\\\\\n"
  "\\hline\n";
  for (TabularResult::Table::const_iterator i=rows_.begin(); i!=rows_.end(); i++)
  {
    if (i!=rows_.begin()) f<<"\\\\\n";
    for (std::vector<double>::const_iterator j=i->begin(); j!=i->end(); j++)
    {
      if (j!=i->begin()) f<<" & ";
      f<<*j;
    }
  }
  f<<"\\end{tabular}\n";
}


ResultSet::ResultSet
(
  const ParameterSet& p,
  const std::string& title,
  const std::string& subtitle,
  std::string* date,
  std::string* author
)
: p_(p),
  title_(title),
  subtitle_(subtitle)
{
  if (date) 
    date_=*date;
  else
  {
    using namespace boost::gregorian;
    date_=to_iso_extended_string(day_clock::local_day());
  }
  
  if (author) 
    author_=*author;
  else
  {
    author_=getenv("USER");
  }
}

ResultSet::~ResultSet()
{}

ResultSet::ResultSet(const ResultSet& other)
: ptr_map< std::string, ResultElement>(other),
  p_(other.p_),
  title_(other.title_),
  subtitle_(other.subtitle_),
  author_(other.author_),
  date_(other.date_)
{
}


void ResultSet::transfer(const ResultSet& other)
{
  ptr_map< std::string, ResultElement>::operator=(other);
  p_=other.p_;
  title_=other.title_;
  subtitle_=other.subtitle_;
  author_=other.author_;
  date_=other.date_;
}


void ResultSet::writeLatexFile(const boost::filesystem::path& file) const
{
  std::ofstream f(file.c_str());
  f<<
  "\\documentclass[a4paper,10pt]{scrartcl}\n";
  for (ResultSet::const_iterator i=begin(); i!=end(); i++)
  {
    i->second->writeLatexHeaderCode(f);
  }    
  f<<
  "\\begin{document}\n"
  "\\title{"<<title_<<"}\n"
  "\\subtitle{"<<subtitle_<<"}\n"
  "\\date{"<<date_<<"}\n"
  "\\author{"<<author_<<"}\n"
  "\\maketitle\n"
  "\\section{Input Parameters}\n";
  
  f<<p_.latexRepresentation();
  
  f<<
  "\\section{Numerical Result Summary}\n";
  for (ResultSet::const_iterator i=begin(); i!=end(); i++)
  {
    f << "\\paragraph{" << i->first << "}\n";
    f << i->second->shortDescription() << " \\\\ ";
    i->second->writeLatexCode(f);
    f << endl;
  }
  f<<
  "\\end{document}\n";
}

}



