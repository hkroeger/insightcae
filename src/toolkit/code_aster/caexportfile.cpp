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

#include "base/tools.h"
#include "boost/foreach.hpp"
#include <boost/iterator/iterator_concepts.hpp>

#include <fstream>

namespace fs=boost::filesystem;

namespace insight
{

CAExportFile::CAExportFile(const boost::filesystem::path& commFile, std::string version, int t_max, int mem_max)
  : boost::filesystem::path(commFile.parent_path()/(commFile.filename().stem().string()+".export")),
    t_max_(t_max),
    mem_max_(mem_max),
    version_(version),
    commFile_(commFile)
{
  setMessFile(commFile.parent_path()/(commFile.filename().stem().string()+".mess.txt"));
  setRMedFile(commFile.parent_path()/(commFile.filename().stem().string()+".rmed"));
}

CAExportFile::~CAExportFile()
{
}

const boost::filesystem::path &CAExportFile::commFilePath() const
{
  return commFile_;
}

void CAExportFile::writeFile(const fs::path& exportFileName) const
{
  fs::path fn=*this;
  if (!exportFileName.empty())
    fn=exportFileName;

  long int mem_max = mem_max_;
  if (mem_max<1)
  {
    MemoryInfo mi;
    mem_max = mi.memTotal_/1024/1024; // MB
  }

  std::ofstream f(fn.string());
  f
  <<"P actions make_etude\n"
  <<"P consbtc oui\n"
  <<"P corefilesize unlimited\n"
  <<"P cpresok RESNOOK\n"
  <<"P debug nodebug\n"
  <<"P soumbtc oui\n"
  <<"P facmtps 1\n"

  <<"P mode interactif\n"
  <<"P version " << version_ << "\n"
  <<"P mpi_nbcpu "<<np_mpi_<<"\n"
  <<"P mpi_nbnoeud 1\n"
  <<"P ncpus "<<np_omp_<<"\n"
  <<"A args\n"
  <<"A memjeveux " << (mem_max/8/np_mpi_) << "\n"
  <<"A tpmax " << t_max_ << "\n"
  <<"F comm " << commFile_.c_str() << " D  1\n";
  
  for (const FileList::value_type& item: mmedFiles_)
  {
    f << "F mmed " << item.second.string() << " D  " << item.first << "\n";
  }
  
  if (rmedFile_.get())
  {
    f << "F rmed " << rmedFile_->string() << " R  80\n";
  }
  
  if (messFile_.get())
  {
    f << "F mess " << messFile_->string() << " R  6\n";
  }
  
}


}
