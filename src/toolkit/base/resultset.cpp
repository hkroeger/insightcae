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

ResultElement::ResultElement(const string& shortDesc, const string& longDesc, const string& unit)
: shortDescription_(shortDesc),
  longDescription_(longDesc),
  unit_(unit)
{}

ResultElement::~ResultElement()
{}


void ResultElement::writeLatexHeaderCode(std::ostream& f) const
{
}

void ResultElement::writeLatexCode(ostream& f) const
{
}


ScalarResult::ScalarResult(const double& value, const string& shortDesc, const string& longDesc, const string& unit)
: NumericalResult< double >(value, shortDesc, longDesc, unit)
{}

void ScalarResult::writeLatexCode(ostream& f) const
{
  f.setf(ios::fixed,ios::floatfield);
  f.precision(3);
  f << value_;
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

void ResultSet::writeLatexFile(const boost::filesystem::path& file) const
{
  std::ofstream f(file.c_str());
  f<<
  "\\documentclass[a4paper,10pt]{scrartcl}\n"
  "\\begin{document}\n"
  "\\title{"<<title_<<"}\n"
  "\\subtitle{"<<subtitle_<<"}\n"
  "\\date{"<<date_<<"}\n"
  "\\author{"<<author_<<"}\n"
  "\\maketitle\n"
  "\\section{Input Parameters}\n";
  //"\\begin{enumerate}\n"
  f<<p_.latexRepresentation();
  
  f<<
  //"\\end{enumerate}\n"
  "\\section{Numerical Result Summary}\n"
  "\\begin{tabular}{lcl}\n"
  "Description of Quantity & Short Name & Value \\\\\n"
  "\\hline\n";
  for (ResultSet::const_iterator i=begin(); i!=end(); i++)
  {
    f << i->second->shortDescription() <<" & " << i->first << " & ";
    i->second->writeLatexCode(f);
    f << i->second->unit() << "\\\\" <<endl;
  }
  f<<
  "\\end{tabular}\n"
  "\\end{document}\n";
}

}



