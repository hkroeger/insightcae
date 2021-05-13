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

#include "blockmesh_cylwedge.h"

#include "base/tools.h"
#include "base/units.h"

#include "openfoam/ofes.h"
#include "openfoam/openfoamcase.h"

#include "cadfeatures.h"
#include "datum.h"

namespace insight
{
namespace bmd
{




ParameterSet_VisualizerPtr blockMeshDict_CylWedge_visualizer()
{
    return ParameterSet_VisualizerPtr( new blockMeshDict_CylWedge_ParameterSet_Visualizer );
}

addStandaloneFunctionToStaticFunctionTable(OpenFOAMCaseElement, blockMeshDict_CylWedge, visualizer, blockMeshDict_CylWedge_visualizer);


void blockMeshDict_CylWedge_ParameterSet_Visualizer::recreateVisualizationElements(const std::string& featureName)
{
  CAD_ParameterSet_Visualizer::recreateVisualizationElements();

  Parameters p(currentParameters());

  OpenFOAMCase oc(OFEs::getCurrentOrPreferred());
  blockMeshDict_CylWedge bcw( oc, currentParameters() );

  auto dom =
      cad::Revolution::create(
        cad::Quad::create(
          cad::matconst(p.geometry.p0 + bcw.er_*p.geometry.d*0.5),
          cad::matconst(bcw.er_*(p.geometry.D - p.geometry.d)*0.5),
          cad::matconst(bcw.ex_*p.geometry.L)
          ),
        cad::matconst(p.geometry.p0),
        cad::matconst(p.geometry.ex),
        cad::scalarconst(p.geometry.wedge_angle*SI::deg),
        true
        )
      ;

  addFeature( featureName,
              cad::Compound::create(cad::CompoundFeatureList({dom})),
              AIS_WireFrame
              );
}



void blockMeshDict_CylWedge_ParameterSet_Visualizer::recreateVisualizationElements()
{
  recreateVisualizationElements("blockMeshDict_CylWedge");
}

}
}
