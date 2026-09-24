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

#include "oftimedirectories.h"
#include "base/analysis.h"
#include "base/cppextensions.h"
#include "base/linearalgebra.h"
#include "base/boost_include.h"
#include "base/progressdisplayer/textprogressdisplayer.h"
#include "base/tools.h"
#include "base/translations.h"

#include "base/units.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/ofes.h"
#include "openfoam/solveroutputanalyzer.h"
#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/createpatch.h"

#include "boost/regex.hpp"
#include "boost/iostreams/filtering_stream.hpp"
#include "boost/iostreams/filter/gzip.hpp"

#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <map>
#include <cmath>
#include <limits>

#include "vtkSTLReader.h"
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkPolyDataReader.h"
#include "vtkCellData.h"
#include "base/warningdispatcher.h"

using namespace std;
using namespace arma;
using namespace boost;
using namespace boost::filesystem;

namespace insight
{


TimeDirectoryList listTimeDirectories(
    const boost::filesystem::path& dir,
    const boost::filesystem::path& fta )
{
    TimeDirectoryList list;
    if ( exists( dir ) )
    {
        directory_iterator end_itr; // default construction yields past-the-end
        for ( directory_iterator itr( dir );
                itr != end_itr;
                ++itr )
        {
            if ( is_directory(itr->status()) )
            {
              auto td = itr->path();
              std::string fn=td.filename().string();
              if (isNumber(fn))
              {
                  double time = toNumber<double>(fn);
                  if (!fta.empty())
                  {
                    if (exists(td/fta))
                      list[time]=td/fta;
                  }
                  else
                    list[time]=td;
              }
            }
        }
    }
    return list;
}

std::string getLatestTimeDirectory(
    const boost::filesystem::path& dir )
{
  auto tds = listTimeDirectories(dir);
  if (tds.size()<1)
  {
    throw insight::Exception(
          _("No time directories present in case %s"),
          dir.string().c_str() );
  }
  return tds.rbegin()->second.string();
}


}
