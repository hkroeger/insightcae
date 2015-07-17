{% load kdev_filters %}
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

#include "{{ output_file_header }}"
#include "base/factory.h"
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/blockmesh.h"

#include "refdata.h"

using namespace std;
using namespace boost;
using namespace boost::assign;

namespace insight {
  
defineType({{ name }});
addToFactoryTable(Analysis, {{ name }}, NoParameters);
  
{{ name }}::{{ name }}(const NoParameters&)
: OpenFOAMAnalysis("{{ name }}", "OpenFOAM based numerical simulation")
  // default values for derived parameters
{
}

  
ParameterSet {{ name }}::defaultParameters() const
{
  ParameterSet p(OpenFOAMAnalysis::defaultParameters());
  p.merge(Parameters::makeDefault());
  return p;
}

void {{ name }}::calcDerivedInputData()
{
  insight::OpenFOAMAnalysis::calcDerivedInputData();
  Parameters p(*parameters_);
  
  //reportIntermediateParameter("L", L_, "total domain length", "m");
}


void {{ name }}::createMesh(insight::OpenFOAMCase& cm)
{
  Parameters p(*parameters_);
  
  cm.insert(new MeshingNumerics(cm));
  cm.createOnDisk(executionPath());
  
  using namespace insight::bmd;
  
  std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "wall");
  
  // create patches
  // Patch& in_coflow = 	bmd->addPatch(p_in_coflow_, new Patch());
  
  double Lx=1., Ly=1., Lz=1.;
  
  pts = boost::assign::map_list_of 
    
      (0, 	vec3(-Lx/2., -Ly/2., -Lz/2.))
      (1, 	vec3( Lx/2., -Ly/2., -Lz/2.))
      (2, 	vec3( Lx/2.,  Ly/2., -Lz/2.))
      (3, 	vec3(-Lx/2.,  Ly/2., -Lz/2.))
      (4, 	vec3(-Lx/2., -Ly/2.,  Lz/2.))
      (5, 	vec3( Lx/2., -Ly/2.,  Lz/2.))
      (6, 	vec3( Lx/2.,  Ly/2.,  Lz/2.))
      (7, 	vec3(-Lx/2.,  Ly/2.,  Lz/2.))
      .convert_to_container<std::map<int, Point> >()
  ;

  
  // create patches
  Patch& sides 	= 	bmd->addPatch("sides", new Patch());
  Patch& top 	= 	bmd->addPatch("top", new Patch("symmetryPlane"));
  Patch& bottom = 	bmd->addPatch("bottom", new Patch("symmetryPlane"));
  
  {
    std::map<int, Point>& p = pts;
    
    {
      Block& bl = bmd->addBlock
      (
	new Block(P_8(
	    p[0], p[1], p[2], p[3],
	    p[4], p[5], p[6], p[7]
	  ),
	  nx, ny, nz
	)
      );
      sides.addFace(bl.face("0154"));
      sides.addFace(bl.face("1265"));
      sides.addFace(bl.face("2376"));
      sides.addFace(bl.face("0473"));
      bottom.addFace(bl.face("0321"));
      top.addFace(bl.face("4567"));
    }
  }
  
  cm.insert(bmd.release());

  cm.createOnDisk(dir);
  cm.executeCommand(dir, "blockMesh");  
  cm.executeCommand(executionPath(), "renumberMesh", list_of("-overwrite"));  
  
}

void {{ name }}::createCase(insight::OpenFOAMCase& cm)
{
  Parameters p(*parameters_);
  
  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);
    
  cm.insert(new simpleFoamNumerics(cm));
  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters() ));
  
  cm.insert(new SimpleBC(cm, "top", boundaryDict, "symmetryPlane"));
  cm.insert(new SimpleBC(cm, "bottom", boundaryDict, "symmetryPlane"));
  cm.insert(new VelocityInletBC(cm, "sides", boundaryDict, VelocityInletBC::Parameters() ));
  
  insertTurbulenceModel(cm, p.get<SelectionParameter>("fluid/turbulenceModel").selection());
}




ResultSetPtr {{ name }}::evaluateResults(OpenFOAMCase& cmp)
{
  Parameters p(*parameters_);
  
  ResultSetPtr results=insight::OpenFOAMAnalysis::evaluateResults(cmp);
    
    
  boost::ptr_vector<sampleOps::set> sets;
  
  sets.push_back(new sampleOps::uniformLine(sampleOps::uniformLine::Parameters()
    .set_name("axial")
    .set_start( vec3(0, 0, 0))
    .set_end(   vec3(1, 0, 0))
    .set_np(20)
  ));
  
  sample(cmp, executionPath(), 
     list_of<std::string>("p")("U"),
     sets
  );
  
  sampleOps::ColumnDescription cd;
  arma::mat data = dynamic_cast<sampleOps::uniformLine*>(&sets[0])
    ->readSamples(cmp, executionPath(), &cd);

    
  {
    int c=cd["U"].col;
    
    arma::mat U(join_rows( data.col(0)/p.geometry.D, data.col(c) ));
    
    addPlot
    (
      results, executionPath(), "chartMeanVelocity_axial",
      "x/D", "<U>/U0",
      list_of
      (PlotCurve(U, "w l lt 1 lc 1 lw 4 t 'Axial'"))
      (PlotCurve(refdata_U, "w l lt 2 lc 1 t 'Axial (Exp., J-Mode)'"))
      ,
      "Profiles of averaged velocities at centerline"
    );
    
  }

  return results;
}

}

