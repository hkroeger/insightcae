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


#ifndef INSIGHT_TEMPLATEFILE_H
#define INSIGHT_TEMPLATEFILE_H

#include <string>
#include <istream>
#include <ostream>

#include <boost/filesystem.hpp>

#include "base/stringconv.h"

namespace insight {

class TemplateFile
    : public std::string
{
public:
  TemplateFile(const std::string& hardCodedTemplate);
  TemplateFile(std::istream& in);
  TemplateFile(const boost::filesystem::path& in);

  void replace(const std::string& keyword, const std::string& content);

  template<class T>
  void replaceValue(const std::string& keyword, const T& content)
  {
      replace(keyword, toString<T>(content));
  }

  void write(std::ostream& os) const;
  void write(const boost::filesystem::path& outfile) const;
};

}

#endif // INSIGHT_TEMPLATEFILE_H
