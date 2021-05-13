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


#include "blockmesh_cylwedgeortho.h"
#include "base/tools.h"
#include "base/units.h"

#include "cadfeatures.h"
#include "datum.h"

#include "BRepBuilderAPI_NurbsConvert.hxx"



namespace insight
{
namespace bmd
{


ParameterSet_VisualizerPtr create_blockMeshDict_CylWedgeOrtho_ParameterSet_Visualizer()
{
    return ParameterSet_VisualizerPtr( new blockMeshDict_CylWedgeOrtho_ParameterSet_Visualizer );
}

addStandaloneFunctionToStaticFunctionTable(OpenFOAMCaseElement, blockMeshDict_CylWedgeOrtho, visualizer, create_blockMeshDict_CylWedgeOrtho_ParameterSet_Visualizer);





void blockMeshDict_CylWedgeOrtho_ParameterSet_Visualizer::recreateVisualizationElements(const std::string& blockMeshName)
{
  CAD_ParameterSet_Visualizer::recreateVisualizationElements();

  Parameters p(currentParameters());

  auto wsc =
      insight::cad::Import::create( p.geometry.wedge_spine_curve->filePath() );

  wsc->checkForBuildDuringAccess(); // force rebuild
  auto el = wsc->allEdgesSet();

  if (el.size()!=1)
    throw insight::Exception(
        boost::str(boost::format("Spine curve should contain only one single edge! (It actually contains %d edges)")
                   % el.size() )
        );

  TopoDS_Edge e= TopoDS::Edge(wsc->edge(*el.begin()));

  arma::mat e_ax = p.geometry.ex/arma::norm(p.geometry.ex, 2);
  arma::mat r0 = vec3(BRep_Tool::Pnt(TopExp::FirstVertex(e))) - p.geometry.p0;
  arma::mat r1 = vec3(BRep_Tool::Pnt(TopExp::LastVertex(e))) - p.geometry.p0;
  arma::mat z0=arma::dot(r0, e_ax)*e_ax;
  r0 -= z0;
  r1 -= arma::dot(r1, e_ax)*e_ax;

  double d0=arma::norm(r0,2)*2.;
  double d1=arma::norm(r1,2)*2.;

  double D0=std::min(d0,d1);
  double D1=std::max(d0,d1);


  auto surf = cad::Transform::create_translate(
          cad::FillingFace::create_set(
          wsc->allEdges().clone(),
          cad::Transform::create_translate(
            wsc,
            cad::matconst(p.geometry.L*p.geometry.ex)
            )->allEdges().clone()
          ),
          cad::matconst(-z0)
        );

  auto wedge = cad::Revolution::create(
        surf,
        cad::matconst(p.geometry.p0),
        cad::matconst(p.geometry.ex),
        cad::scalarconst(p.geometry.wedge_angle*SI::deg),
        false
        );




  std::vector<cad::FeaturePtr> feats;

  if (const auto* ii = boost::get<Parameters::geometry_type::inner_interface_extend_type>(&p.geometry.inner_interface))
  {
    if ( fabs(ii->z0) > Precision::Confusion())
    {
      feats.push_back(
          cad::Cylinder::create_hollow(
            cad::matconst(p.geometry.p0),
            cad::matconst(p.geometry.p0 + ii->z0*p.geometry.ex),
            cad::scalarconst(ii->distance*2. + D0),
            cad::scalarconst(D0),
            false, false
            )
          );
    }
    if ( fabs(p.geometry.L - ii->z1) > Precision::Confusion() )
    {
      feats.push_back(
          cad::Cylinder::create_hollow(
            cad::matconst(p.geometry.p0 + ii->z1*p.geometry.ex),
            cad::matconst(p.geometry.p0 + p.geometry.L*p.geometry.ex),
            cad::scalarconst(ii->distance*2. + D0),
            cad::scalarconst(D0),
            false, false
            )
          );
    }
  }


  if (const auto* ii = boost::get<Parameters::geometry_type::outer_interface_extend_type>(&p.geometry.outer_interface))
  {
    if ( fabs(ii->z0)>Precision::Confusion())
    {
      feats.push_back(
        cad::Cylinder::create_hollow(
          cad::matconst(p.geometry.p0),
          cad::matconst(p.geometry.p0 + ii->z0*p.geometry.ex),
          cad::scalarconst(D1),
          cad::scalarconst(D1 - ii->distance*2.),
          false, false
          )
        );
    }
    if ( fabs(p.geometry.L - ii->z1) > Precision::Confusion() )
    {
      feats.push_back(
        cad::Cylinder::create_hollow(
          cad::matconst(p.geometry.p0 + ii->z1*p.geometry.ex),
          cad::matconst(p.geometry.p0 + p.geometry.L*p.geometry.ex),
          cad::scalarconst(D1),
          cad::scalarconst(D1 - ii->distance*2.),
          false, false
          )
        );
    }
  }

  if (feats.size()>0)
  {
    wedge = cad::BooleanSubtract::create(wedge, cad::Compound::create(feats));
//    feats.push_back(wedge);
//    wedge = cad::Compound::create(feats);
  }

  addFeature( blockMeshName,
              wedge,
              AIS_WireFrame
              );
}

void blockMeshDict_CylWedgeOrtho_ParameterSet_Visualizer::recreateVisualizationElements()
{
  recreateVisualizationElements("blockMeshDict_CylWedgeOrtho");
}

}
}
