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

#include "latextools.h"

#include <string>

#include "boost/algorithm/string.hpp"

using namespace boost;

namespace insight
{

std::string cleanSymbols(const std::string& s)
{
  std::string result(s);
  boost::replace_all(result, "_", "\\_");
  boost::replace_all(result, "#", "\\#");
  boost::replace_all(result, "^", "\\textasciicircum");
  boost::replace_all(result, "[", "{[}");
  boost::replace_all(result, "]", "{]}");
  return result;
}

boost::filesystem::path cleanLatexImageFileName(const boost::filesystem::path& s)
{
  std::string stem=s.stem().string();
  replace_all(stem, ".", "_");
  return s.parent_path()/(stem+s.extension().string());
}

}
