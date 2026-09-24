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


#include "temporaryfile.h"
#include "base/exception.h"

#include <cstdlib>

using namespace std;
using namespace boost::filesystem;

namespace insight
{

std::unique_ptr<GlobalTemporaryDirectory> GlobalTemporaryDirectory::td_;


GlobalTemporaryDirectory::GlobalTemporaryDirectory()
  : boost::filesystem::path
    (
      absolute
      (
        unique_path
        (
          temp_directory_path() / "insightcae-%%%%%%"
        )
      )
    )
{
  create_directories(*this);
  permissions(*this, owner_all);
}

const GlobalTemporaryDirectory &GlobalTemporaryDirectory::path()
{
  if (!td_)
    td_.reset( new GlobalTemporaryDirectory );
  return *td_;
}

void GlobalTemporaryDirectory::clear()
{
  td_.reset();
}


GlobalTemporaryDirectory::~GlobalTemporaryDirectory()
{
  remove_all(*this);
}



TemporaryFile::TemporaryFile
(
    const std::string& fileNameModel,
    const boost::filesystem::path& baseDir
)
  : tempFilePath_( boost::filesystem::unique_path(
             (baseDir.empty() ? boost::filesystem::temp_directory_path() : baseDir)
             /
             fileNameModel
             ) )
{
    dbg() << tempFilePath_ << std::endl;
}


TemporaryFile::~TemporaryFile()
{
  if (stream_)
      stream_.reset();

  if (!getenv("INSIGHT_KEEPTEMPORARYFILES"))
  {
    if (boost::filesystem::exists(tempFilePath_))
    {
        dbg()<<"removing file "<<tempFilePath_<<std::endl;
      boost::filesystem::remove(tempFilePath_);
    }
  }
}

std::ostream& TemporaryFile::stream()
{
    if (!stream_)
        stream_=std::make_unique<std::ofstream>(path().string());
    return *stream_;
}

void TemporaryFile::closeStream()
{
    stream_.reset();
}


const boost::filesystem::path& TemporaryFile::path() const
{
  return tempFilePath_;
}


}
