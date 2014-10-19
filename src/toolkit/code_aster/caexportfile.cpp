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

#include "caexportfile.h"

#include "boost/foreach.hpp"
#include <boost/iterator/iterator_concepts.hpp>

#include <fstream>

namespace insight
{

CAExportFile::CAExportFile(const boost::filesystem::path& commFile, std::string version, int t_max, int mem_max)
: t_max_(t_max),
  mem_max_(mem_max),
  version_(version),
  commFile_(commFile)
{
  setMessFile("output.mess");
  setRMedFile("results.med");
}

CAExportFile::~CAExportFile()
{
}

void CAExportFile::writeFile(const boost::filesystem::path& location)
{
  std::ofstream f(location.c_str());
  f
  <<"P actions make_etude\n"
  <<"P mode interactif\n"
  <<"P version " << version_ << "\n"
  <<"A memjeveux " << mem_max_ << "\n"
  <<"A tpmax " << t_max_ << "\n"
  <<"F comm " << commFile_.c_str() << " D  1\n";
  
  BOOST_FOREACH(const FileList::value_type& item, mmedFiles_)
  {
    f << "F mmed " << item.second.c_str() << " D  " << item.first << "\n";
  }
  
  if (rmedFile_.get())
  {
    f << "F rmed " << rmedFile_->c_str() << " R  80\n";
  }
  
  if (messFile_.get())
  {
    f << "F mess " << messFile_->c_str() << " R  6\n";
  }
  
}


}