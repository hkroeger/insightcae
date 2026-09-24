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


#include "sharedpathlist.h"
#include "base/exception.h"
#include "base/boost_include.h"
#include "base/filesystemtools.h"

#include <cstdlib>
#include <boost/algorithm/string.hpp>
#include "boost/range/adaptor/reversed.hpp"

#ifdef WIN32
#include <windows.h>
#endif

using namespace std;
using namespace boost;
using namespace boost::filesystem;

namespace insight
{


SharedPathList::SharedPathList()
{
  CurrentExceptionContext ec("building list of shared paths");


  insertPathRelativeToCurrentExecutable(
              boost::filesystem::path("..")/"share"/"insight" );

  if (char *var_usershareddir=getenv("INSIGHT_USERSHAREDDIR"))
  {
    push_back(var_usershareddir);
  }
  else
  {
    if (char *userdir = getenv(
#ifdef WIN32
                "USERPROFILE"
#else
                "HOME"
#endif
                ))
    {
      push_back( path(userdir)/".insight"/"share" );
    }
  }

  if (char *var_globalshareddir=getenv("INSIGHT_GLOBALSHAREDDIRS"))
  {
    std::vector<string> globals;
    split(globals, var_globalshareddir,
#ifdef WIN32
          is_any_of(";") // colon collides with drive letter in windows
#else
          is_any_of(":")
#endif
          );
    for (const string& s: globals) push_back(s);
  }
  else
  {
    push_back( path("/usr/share/insight") );
  }
}

SharedPathList &SharedPathList::global()
{
    static SharedPathList spl;
    return spl;
}



path SharedPathList::getSharedFilePath(const path& file, bool* foundPtr)
{
  if (foundPtr) *foundPtr=true;
  for( auto& p: boost::adaptors::reverse(*this))
  {
    if (exists(p/file))
      return p/file;
  }

  // nothing found
  if (foundPtr)
  {
    *foundPtr=false;
    return boost::filesystem::path();
  }
  else
  {
    throw insight::Exception(
        std::string("Requested shared file ")
         +file.string()
         +" not found either in global nor user shared directories"
        );
  }
}

void SharedPathList::insertIfNotPresent(const path& spr)
{
  path sp = boost::filesystem::absolute(spr);
  if (std::find(begin(), end(), sp) == end())
  {
    insight::dbg()<<"Extend search path: "<<sp.string()<<std::endl;
    push_back(sp);
  }
  else
  {
    insight::dbg()<<"Already included in search path: "<<sp.string()<<std::endl;
  }
}

void SharedPathList::insertFileDirectoyIfNotPresent(const path& sp)
{
  if (boost::filesystem::is_directory(sp))
  {
    insertIfNotPresent(sp);
  }
  else
  {
    insertIfNotPresent(sp.parent_path());
  }
}

void SharedPathList::insertPathRelativeToCurrentExecutable(
        const boost::filesystem::path &relPath)
{
    using namespace boost::filesystem;
#if defined(WIN32)
    char buf[MAX_PATH];
    GetModuleFileName(NULL, buf, MAX_PATH);
    path exe(buf);
    path dir=exe.parent_path() / relPath;
    insight::dbg()<<exe<<" "<<dir<<std::endl;
    insertIfNotPresent(dir);
#else
    path link("/proc/self/exe");
    if (is_symlink(link))
    {
        insight::dbg()<<read_symlink(link)<<std::endl;
        insertIfNotPresent(
                    read_symlink(link).parent_path()
                    / relPath );
    }
    else
    {
        insight::dbg() << "skipping addition of path <executable dir>/"+relPath.string()
                          +" because operating system does not provide symlink in /proc/self/exe."
                          +" Make sure, the variable INSIGHT_GLOBALSHAREDDIRS contains all the appropriate paths."
                       <<std::endl;
    }
#endif
}

boost::filesystem::path SharedPathList::findFirstWritableLocation(
        const boost::filesystem::path &subPath) const
{
    insight::SharedPathList paths;
    for ( const bfs_path& p: paths )
    {
        insight::dbg()<<"checking, if "<<p.string()<<" is writable."<<std::endl;
        if ( insight::isInWritableDirectory(p) )
        {
            return p / subPath;
        }
    }
    return bfs_path();
}


}
