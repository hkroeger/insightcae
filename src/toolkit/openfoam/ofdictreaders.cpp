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

#include "ofdictreaders.h"
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


std::string readSolverName(const boost::filesystem::path& ofcloc)
{
  OFDictData::dict controlDict;
  std::ifstream cdf( (ofcloc/"system"/"controlDict").c_str() );
  readOpenFOAMDict(cdf, controlDict);
  return controlDict.getString("application");
}

int readDecomposeParDict(const boost::filesystem::path& ofcloc)
{
    boost::filesystem::path fn(ofcloc/"system"/"decomposeParDict");
    if (boost::filesystem::exists(fn))
    {
        OFDictData::dict decomposeParDict;
        readOpenFOAMDict(fn, decomposeParDict);
        return decomposeParDict.getInt("numberOfSubdomains");
    }
    else
    {
        return 1;
    }
}

std::string readTurbulenceModelName(const OpenFOAMCase& c, const boost::filesystem::path& ofcloc)
{
  OFDictData::dict RASPropertiesDict;
  std::ifstream cdf( (ofcloc/"constant"/"RASProperties").c_str() );
  readOpenFOAMDict(cdf, RASPropertiesDict);
  //cout<<decomposeParDict<<endl;
  if (c.OFversion()<300)
  {
    return RASPropertiesDict.getString("RASModel");
  }
  else
  {
    return RASPropertiesDict.subDict("RAS").getString("RASModel");
  }
}


}
