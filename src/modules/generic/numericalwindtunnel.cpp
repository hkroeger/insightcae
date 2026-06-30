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

#include "numericalwindtunnel.h"

#include "base/exception.h"
#include "base/units.h"
#include "base/vtktransformation.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/snappyhexmesh.h"
#include "openfoam/paraview.h"
#include "openfoam/blockmeshoutputanalyzer.h"


#include "openfoam/caseelements/basic/pressuregradientsource.h"
#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/caseelements/numerics/steadyincompressiblenumerics.h"
#include "openfoam/caseelements/basic/singlephasetransportmodel.h"
#include "openfoam/caseelements/boundaryconditions/simplebc.h"
#include "openfoam/caseelements/boundaryconditions/pressureoutletbc.h"
#include "openfoam/caseelements/boundaryconditions/symmetrybc.h"
#include "openfoam/caseelements/boundaryconditions/wallbc.h"
#include "openfoam/caseelements/boundaryconditions/velocityinletbc.h"
#include "openfoam/caseelements/boundaryconditions/cyclicggibc.h"

#include "openfoam/caseelements/analysiscaseelements.h"




using namespace arma;
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;


namespace insight 
{


defineType(NumericalWindtunnel);
Analysis::Add<NumericalWindtunnel> addNumericalWindtunnel;


void NumericalWindtunnel::modifyDefaults(insight::ParameterSet& p)
{
    p.setBool("run/potentialinit", true);
}


namespace { boost::mutex mtx; }




NumericalWindtunnel::NumericalWindtunnel(
    const std::shared_ptr<supplementedInputDataBase>& sp )
: OpenFOAMAnalysis(sp)
{}



void NumericalWindtunnel::calcDerivedInputData(ProgressDisplayer& parentProgresss)
{
  reportIntermediateParameter("Lref", sp().Lref_, "reference length of object (bounding box diagonal)", "m");
}




void NumericalWindtunnel::createMesh(insight::OpenFOAMCase& cm, ProgressDisplayer& parentProgress)
{
  path dir = executionPath();
  

  cm.insert(new MeshingNumerics(cm, MeshingNumerics::Parameters()
    .set_np(np())
  ));
  cm.createOnDisk(executionPath());
  
  int nb = cm.insert(new bmd::blockMesh(cm, *sp().blocking))->nBlocks();
  
  cm.createOnDisk(executionPath());
  cm.runBlockMesh(executionPath(), nb, &parentProgress);
    


  snappyHexMeshConfiguration::Parameters shm_cfg;
  
  for (auto& g: sp().geometry_)
  {
      shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(
          new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
        .set_minLevel(p().mesh.lmsurf)
        .set_maxLevel(p().mesh.lxsurf)
        .set_nLayers(p().mesh.nlayer)
        .set_geometry(make_geometryFile(g.second))
        .set_name(g.first)
      )));
  }
  shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(
   new snappyHexMeshFeats::RefinementBox(snappyHexMeshFeats::RefinementBox::Parameters()
    .set_min(vec3(-0.33*sp().l_, -(0.5+0.5)*sp().w_, sp().Ldown_))
    .set_max(vec3(2.0*sp().l_, (0.5+0.5)*sp().w_, sp().Ldown_+1.33*(sp().hup_+sp().dlo_)))
    
    .set_level(p().mesh.boxlevel)
    .set_name("refinement_box")
  )));
  shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::RefinementBox(snappyHexMeshFeats::RefinementBox::Parameters()
    .set_min(vec3(0.8*sp().l_, -(0.5+0.25)*sp().w_, sp().Ldown_))
    .set_max(vec3(1.5*sp().l_, (0.5+0.25)*sp().w_, sp().Ldown_+1.2*(sp().hup_+sp().dlo_)))
    
    .set_level(p().mesh.rearlevel)
    .set_name("refinement_rear")
  )));


  int iref=0;
  for (const Parameters::mesh_type::refinementZones_default_type& rz:
       p().mesh.refinementZones)
  {
    if (const auto* bc =
        boost::get<Parameters::mesh_type::refinementZones_default_type::geometry_box_centered_type>(&rz.geometry))
    {
      arma::mat d = vec3(bc->L, bc->W, bc->H);
      shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::RefinementBox(snappyHexMeshFeats::RefinementBox::Parameters()
        .set_min(bc->pc - 0.5*d)
        .set_max(bc->pc + 0.5*d)

        .set_level(rz.lx)
        .set_name(str(format("refinement_%d")%iref))
      )));
    }
    else if (const auto* b =
        boost::get<Parameters::mesh_type::refinementZones_default_type::geometry_box_type>(&rz.geometry))
    {
      shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::RefinementBox(snappyHexMeshFeats::RefinementBox::Parameters()
        .set_min(b->pmin)
        .set_max(b->pmax)

        .set_level(rz.lx)
        .set_name(str(format("refinement_%d")%iref))
      )));
    }

    iref++;
  }


  shm_cfg.PiM.push_back(sp().PiM_);

  shm_cfg
  .set_tlayer ( p().mesh.tlayer )
  .set_erlayer ( 1.3 )
  ;
  
  snappyHexMesh
  (
    cm, executionPath(),
    shm_cfg,
    true, false, false,
    &parentProgress
  );

  
  resetMeshToLatestTimestep(cm, executionPath(), true);
  
}



void NumericalWindtunnel::createCase(insight::OpenFOAMCase& cm, ProgressDisplayer&)
{

  double turbI=0.01; // Free-stream turbulence
  double turbL=0.001*(sp().hup_+sp().dlo_); // Free-stream turbulence length scale => very low 0.1% of car height

  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

  cm.insert(new steadyIncompressibleNumerics(cm, steadyIncompressibleNumerics::Parameters()
    .set_writeInterval(100.0)
    .set_purgeWrite(0)
    .set_endTime(1000.0)
    .set_deltaT(1)
  ));

  forces::Parameters::patches_type patchList;
  for (auto& g: sp().geometry_)
  {
      std::string ppat="\""+g.first+".*\"";
      patchList.push_back(ppat);

      if (sp().geometry_.size()>1)
      {
          cm.insert(new forces(
              cm, forces::Parameters()
                  .set_rhoInf(p().fluid.rho)
                  .set_patches({ ppat })
                  .set_name("forces_"+g.first)
              ));
      }
  }

  if (p().eval.allForces.includeTopWall)
      patchList.push_back("top");
  if (p().eval.allForces.includeBottomWall)
      patchList.push_back("floor");

  cm.insert(new forces(cm, forces::Parameters()
    .set_rhoInf(p().fluid.rho)
    .set_patches(patchList)
    .set_name(sp().FOname_allObjects)
  ));

  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters()
    .set_nu(p().fluid.nu)
  ));

  if (p().operation.boundaryConditions.sides ==
      Parameters::operation_type::boundaryConditions_type::sideSymmetryPlane)
  {
    cm.insert(new SymmetryBC(cm, "side1", boundaryDict));
  }
  else if (p().operation.boundaryConditions.sides ==
      Parameters::operation_type::boundaryConditions_type::sideOutlet)
  {
      cm.insert(new PressureOutletBC(cm, "side1", boundaryDict));
  }
  // else wall



  if (p().mesh.longitudinalSymmetry
      ||
      (p().operation.boundaryConditions.sides ==
          Parameters::operation_type::boundaryConditions_type::sideSymmetryPlane) )
  {
      cm.insert(new SymmetryBC(cm, "side2", boundaryDict));
  }
  else if (p().operation.boundaryConditions.sides ==
           Parameters::operation_type::boundaryConditions_type::sideOutlet)
  {
      cm.insert(new PressureOutletBC(cm, "side2", boundaryDict));
  }
  // else wall




  if (p().operation.boundaryConditions.bottomWall ==
        Parameters::operation_type::boundaryConditions_type::movingBottomWall)
  {
      cm.insert(new WallBC(cm, "floor", boundaryDict, WallBC::Parameters()
        .set_wallVelocity(vec3(p().operation.v,0,0))
      ));
  }
  else if (p().operation.boundaryConditions.bottomWall ==
        Parameters::operation_type::boundaryConditions_type::bottomSymmetryPlane)
  {
      cm.insert(new SimpleBC(cm, "floor", boundaryDict, "symmetryPlane"));
  }
  // else wall



  if (p().operation.boundaryConditions.topWall ==
      Parameters::operation_type::boundaryConditions_type::movingTopWall)
  {
      cm.insert(new WallBC(cm, "top", boundaryDict, WallBC::Parameters()
                               .set_wallVelocity(vec3(p().operation.v,0,0))
                           ));
  }
  else if (p().operation.boundaryConditions.topWall ==
         Parameters::operation_type::boundaryConditions_type::topSymmetryPlane)
  {
      cm.insert(new SimpleBC(cm, "top", boundaryDict, "symmetryPlane"));
  }
  // else wall


  if (boost::get<Parameters::operation_type::inflow_homogeneousInflow_type>(
          &p().operation.inflow))
  {
      cm.insert(new PressureOutletBC(cm, "outlet", boundaryDict));
      cm.insert(new VelocityInletBC(cm, "inlet", boundaryDict, VelocityInletBC::Parameters()
          .set_velocity( FieldData::uniformSteady(p().operation.v,0,0) )
          .set_turbulence(turbulenceBC::turbulenceBCPtr(new turbulenceBC::uniformIntensityAndLengthScale(
               turbulenceBC::uniformIntensityAndLengthScale::Parameters()
                .set_I(turbI)
                .set_l(turbL)
            )))
      ));
  }
  else if (boost::get<Parameters::operation_type::inflow_cyclicBC_type>(
          &p().operation.inflow))
  {
      auto L = vec3X(sp().Lupstream_+sp().l_+sp().Ldownstream_);

      cm.insert(new CyclicGGIBC(
          cm, "inlet", boundaryDict, CyclicGGIBC::Parameters()
              .set_separationOffset(L)
              .set_shadowPatch("outlet")
              .set_zone("inlet")
          ));
      cm.insert(new CyclicGGIBC(
          cm, "outlet", boundaryDict, CyclicGGIBC::Parameters()
              .set_separationOffset(-L)
              .set_shadowPatch("inlet")
              .set_zone("outlet")
          ));

      cm.insert(new PressureGradientSource(
          cm,
          PressureGradientSource::Parameters()
              .set_Ubar(vec3(p().operation.v, 0, 0))
              .set_name("forceMeanVelocity")
          ));
  }

  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters() );

  insertTurbulenceModel(cm, p().fluid.turbulenceModel);

}





    
}
