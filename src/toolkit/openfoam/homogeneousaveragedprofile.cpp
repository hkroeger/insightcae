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

#include "homogeneousaveragedprofile.h"
#include "openfoam/sampling.h"
#include "openfoam/ofmiscutils.h"
#include "base/resultelements/chart.h"
#include "boost/ptr_container/ptr_vector.hpp"
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


HomogeneousAveragedProfile::HomogeneousAveragedProfile(
    const std::shared_ptr<supplementedInputDataBase>& sp )
: AnalysisWithParameters(sp)
{}


ResultSetPtr HomogeneousAveragedProfile::operator()(ProgressDisplayer& /*displayer*/)
{
  OpenFOAMCase cm(p().OFEname);
  
  arma::mat xs;
  
  if ( Parameters::grading_type::none == p().grading )
  {
    xs=linspace(0., 1., p().np);
  }
  else if ( Parameters::grading_type::towardsEnd == p().grading )
  {
    xs=cos(0.5*M_PI*(linspace(0., 1., p().np)-1.0));
  }
  else if ( Parameters::grading_type::towardsStart == p().grading )
  {
    xs=1.0 - cos(0.5*M_PI*linspace(0., 1., p().np));
  }

  
  arma::mat pts = arma::trans(
      p().L * arma::trans(xs)
     +
      p().p0 * arma::ones(1,p().np)
  );
  
  boost::ptr_vector<sampleOps::set> sets;
  sets.push_back(new sampleOps::linearAveragedPolyLine(sampleOps::linearAveragedPolyLine::Parameters()
    .set_points( pts )
    .set_dir1(p().homdir1)
    .set_dir2(p().homdir2)
    .set_nd1(p().n_homavg1)
    .set_nd2(p().n_homavg2)
    .set_name(p().profile_name)
  ));

  auto casepath = p().casepath->expandedFilePath();

  sample(
      cm, casepath,
      std::container_type_cast<std::vector<std::string> >(
          p().fields ),
      sets);
      
  sampleOps::ColumnDescription cd;
  arma::mat data = dynamic_cast<sampleOps::linearAveragedPolyLine*>(&sets[0])
    -> readSamples(cm, casepath, &cd);

  auto results = createResultSet();
  hierarchicalData::Ordering so;
  
  for (const std::string& fieldname: p().fields)
  {
    int c=cd[fieldname].col;
    int ncmpt=cd[fieldname].ncmpt;
    
    PlotCurveList crvs;
    for (int i=0; i<ncmpt; i++)
    {
      
      std::string cmptname = getOpenFOAMComponentLabel(i, ncmpt);
      
      std::string lxcmptname="";
      if (cmptname!="") lxcmptname="_{"+cmptname+"}";
      
      crvs.push_back(PlotCurve(data.col(0), data.col(c+i), fieldname+cmptname, "w l t '$"+fieldname+lxcmptname+"$'"));
    }
    
    addPlot
    (
      *results, casepath, "profiles_"+fieldname,
      "$x / m$", fieldname,
      crvs,
      "Profiles of field "+fieldname
    ) 
    .setOrder(so.next());
    
  }
  
  return results;
}

defineType(HomogeneousAveragedProfile);
Analysis::Add<HomogeneousAveragedProfile> addHomogeneousAveragedProfile;

}
