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

#ifndef INSIGHT_CAEXPORTFILE_H
#define INSIGHT_CAEXPORTFILE_H

#include "base/boost_include.h"
#include <vector>
#include <map>

namespace insight {

class CAExportFile
    : public boost::filesystem::path
{
  
public:
  std::string
    MESS="mess",
    RMED="rmed",
    MMED="mmed"
  ;

  struct FileInfo
  {
    std::string fileType;
    boost::filesystem::path filePath;
  };
  typedef std::map<int, FileInfo> FileList;
  
protected:
  int t_max_, mem_max_;
  std::string version_;
  
  boost::filesystem::path commFile_;
  FileList inputFiles_, outputFiles_;
  
  int np_omp_, np_mpi_;
  
public:
    CAExportFile(const boost::filesystem::path& commFile, std::string version="stable", int t_max=7*24*60*60, int mem_max=0 /*MB*/);
    ~CAExportFile();

    void setNP(int np_omp=1, int np_mpi=1);

    void setMessFile(const boost::filesystem::path& fn);
    void setRMedFile(const boost::filesystem::path& fn);
    void addMeshMedFile(const boost::filesystem::path& fn, int unit=-1);

    std::set<boost::filesystem::path> outputFiles(const std::string& fileType="mmed") const;

    const boost::filesystem::path& commFilePath() const;

    boost::filesystem::path RMedFile() const;

    virtual void writeFile(const boost::filesystem::path& exportFileName="") const;
};
}

#endif // INSIGHT_CAEXPORTFILE_H
