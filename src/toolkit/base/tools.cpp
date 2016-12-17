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

#include "tools.h"
#include <fstream>
#include <cstdlib>
#include <dlfcn.h>

#include "base/boost_include.h"
#include "base/exception.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;


namespace insight
{

SharedPathList::SharedPathList()
{
  char *var_usershareddir=getenv("INSIGHT_USERSHAREDDIR");
  char *var_globalshareddir=getenv("INSIGHT_GLOBALSHAREDDIRS");
  
  if (var_usershareddir) 
  {
    push_back(var_usershareddir);
  }
  else
  {
    char *userdir=getenv("HOME");
    if (userdir)
    {
      push_back( path(userdir)/".insight"/"share" );
    }
  }
  
  if (var_globalshareddir) 
  {
    std::vector<string> globals;
    split(globals, var_globalshareddir, is_any_of(":"));
    BOOST_FOREACH(const string& s, globals) push_back(s);
  }
  else
  {
    push_back( path("/usr/share/insight") );
  }
}

SharedPathList::~SharedPathList()
{
}


path SharedPathList::getSharedFilePath(const path& file)
{
  BOOST_REVERSE_FOREACH( const path& p, *this)
  {
    if (exists(p/file)) 
      return p/file;
  }
  
  // nothing found
  throw insight::Exception(std::string("Requested shared file ")+file.c_str()+" not found either in global nor user shared directories");
  return path();
}

void SharedPathList::insertIfNotPresent(const path& spr)
{
  path sp = boost::filesystem::absolute(spr);
  if (std::find(begin(), end(), sp) == end())
  {
    std::cout<<"Extend search path: "<<sp.string()<<std::endl;
    push_back(sp);
  }
  else
  {
    std::cout<<"Already included in search path: "<<sp.string()<<std::endl;
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




SharedPathList SharedPathList::searchPathList;

}