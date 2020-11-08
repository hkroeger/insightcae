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
  typedef std::map<int, boost::filesystem::path> FileList;
  
protected:
  int t_max_, mem_max_;
  std::string version_;
  
  boost::filesystem::path commFile_;
  FileList mmedFiles_;
  
  std::unique_ptr<boost::filesystem::path> messFile_;
  std::unique_ptr<boost::filesystem::path> rmedFile_;

  int np_omp_, np_mpi_;
  
public:
    CAExportFile(const boost::filesystem::path& commFile, std::string version="stable", int t_max=7*24*60*60, int mem_max=0 /*MB*/);
    ~CAExportFile();

    inline void setNP(int np_omp=1, int np_mpi=1)
    {
      np_omp_=np_omp;
      np_mpi_=np_mpi;
    }
    
    inline void setMessFile(const boost::filesystem::path& fn)
    {
      messFile_.reset(new boost::filesystem::path(fn));
    }
    
    inline void setRMedFile(const boost::filesystem::path& fn)
    {
      rmedFile_.reset(new boost::filesystem::path(fn));
    }

    inline boost::filesystem::path RMedFile() const
    {
      return *rmedFile_;
    }
    
    inline void addMeshMedFile(const boost::filesystem::path& fn, int unit=-1)
    {
      if (unit<0)
      {
	if (mmedFiles_.size()==0) 
	  unit=20;
	else
	{
	  unit=mmedFiles_.rbegin()->first +1;
	}
      }
      mmedFiles_[unit]=fn;
    }

    const boost::filesystem::path& commFilePath() const;

    virtual void writeFile(const boost::filesystem::path& exportFileName="") const;
};
}

#endif // INSIGHT_CAEXPORTFILE_H
