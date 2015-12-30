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
 */

#include "parser.h"
#include "lookuptablescalar.h"
#include "base/boost_include.h"

using namespace boost;

namespace insight 
{
namespace cad 
{
  
LookupTableScalar::LookupTableScalar
(
  const std::string& name, 
  const std::string& keycol, 
  ScalarPtr keyval, 
  const std::string& depcol
)
: name_(name),
  keycol_(keycol),
  keyval_(keyval),
  depcol_(depcol)
{}


double LookupTableScalar::value() const
{
  std::ifstream f( (parser::sharedModelFilePath(name_+".csv")).c_str() );
  std::string line;
  std::vector<std::string> cols;
  getline(f, line);
  boost::split(cols, line, boost::is_any_of(";"));
  int ik=find(cols.begin(), cols.end(), keycol_) - cols.begin();
  int id=find(cols.begin(), cols.end(), depcol_) - cols.begin();
  double tkeyval=*keyval_;
  while (!f.eof())
  {
    getline(f, line);
    boost::split(cols, line, boost::is_any_of(";"));
    double kv=lexical_cast<double>(cols[ik]);
    if (fabs(kv-tkeyval)<1e-6)
    {
      double dv=lexical_cast<double>(cols[id]);
      return dv;
    }
  }
  throw insight::Exception(
      "Table lookup of value "+lexical_cast<std::string>(tkeyval)+
      " in column "+keycol_+" failed!");
  return 0.0;
}

}
}