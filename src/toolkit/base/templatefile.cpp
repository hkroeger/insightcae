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


#include "templatefile.h"
#include "base/fileio.h"

#include <fstream>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost::filesystem;

namespace insight
{

TemplateFile::TemplateFile(const string &hardCodedTemplate)
  : std::string(hardCodedTemplate)
{}

TemplateFile::TemplateFile(std::istream &in)
{
  readStreamIntoString(in, *this);
}

TemplateFile::TemplateFile(const boost::filesystem::path& in)
{
  readFileIntoString(in, *this);
}

void TemplateFile::replace(const string &keyword, const string &content)
{
  boost::replace_all ( *this, "###"+keyword+"###", content );
}

void TemplateFile::write(ostream &os) const
{
  // os.write(this->c_str(), long(this->size()) );
    os << *this << endl;
}

void TemplateFile::write(const path &outfile) const
{
  std::ofstream f(outfile.string()/*, ios::binary*/);
  write(f);
}

}
