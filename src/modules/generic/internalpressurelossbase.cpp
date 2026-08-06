#include "internalpressurelossbase.h"

#include "base/cppextensions.h"
#include "base/exception.h"
#include "base/factory.h"
#include "base/units.h"
#include "boost/algorithm/string/case_conv.hpp"
#include "boost/algorithm/string/join.hpp"
#include "boost/format/format_fwd.hpp"
#include "boost/iterator_adaptors.hpp"
#include "cadfeature.h"
#include "cadgeometryparameter.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_heat.h"
#include "openfoam/caseelements/boundaryconditions/exptdatainletbc.h"
#include "openfoam/caseelements/boundaryconditions/suctioninletbc.h"
#include "openfoam/caseelements/boundaryconditions/velocityinletbc.h"

#include "openfoam/caseelements/basic/limitquantities.h"
#include "openfoam/caseelements/evaluation/calculatetotalpressure.h"
#include "openfoam/ofes.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/blockmesh.h"
#include "openfoam/snappyhexmesh.h"

#include "base/vtktransformation.h"

#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/caseelements/numerics/steadyincompressiblenumerics.h"
#include "openfoam/caseelements/numerics/unsteadyincompressiblenumerics.h"
#include "openfoam/caseelements/numerics/buoyantsimplefoamnumerics.h"
#include "openfoam/caseelements/numerics/buoyantpimplefoamnumerics.h"
#include "openfoam/caseelements/basic/passivescalar.h"
#include "openfoam/caseelements/basic/singlephasetransportmodel.h"
#include "openfoam/caseelements/basic/gravity.h"
#include "openfoam/caseelements/thermophysicalcaseelements.h"
#include "openfoam/caseelements/boundaryconditions/symmetrybc.h"
#include "openfoam/caseelements/boundaryconditions/massflowbc.h"
#include "openfoam/caseelements/boundaryconditions/pressureoutletbc.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_turbulence.h"
#include "openfoam/caseelements/boundaryconditions/wallbc.h"
#include "openfoam/caseelements/analysiscaseelements.h"
#include "openfoam/caseelements/thermodynamics/compressiblesinglephasethermophysicalproperties.h"

#include "cadfeatures.h"
#include "cadfeatures/stl.h"
#include "datum.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <tuple>

using namespace std;
using namespace boost;
using namespace boost::assign;


namespace insight {


defineType(InternalPressureLossBase);


InternalPressureLossBase::InternalPressureLossBase(
    const std::shared_ptr<supplementedInputDataBase>& sp )
    : OpenFOAMAnalysis(sp)
{}




void InternalPressureLossBase::calcDerivedInputData(ProgressDisplayer& /*prg*/)
{
    reportIntermediateParameter("Lx", sp().L_(0), "model size in x direction", "m");
    reportIntermediateParameter("Ly", sp().L_(1), "model size in y direction", "m");
    reportIntermediateParameter("Lz", sp().L_(2), "model size in z direction", "m");
    reportIntermediateParameter("nx", sp().nx_, "initial grid cell numbers in direction x");
    reportIntermediateParameter("ny", sp().ny_, "initial grid cell numbers in direction y");
    reportIntermediateParameter("nz", sp().nz_, "initial grid cell numbers in direction z");

}




void InternalPressureLossBase::createMesh(insight::OpenFOAMCase& cm, ProgressDisplayer& pp)
{
    cm.insert(new MeshingNumerics(cm, MeshingNumerics::Parameters()
                                      .set_np(np())
                                  ));
    cm.createOnDisk(executionPath());

    using namespace insight::bmd;
    std::unique_ptr<blockMesh> bmd(new blockMesh(cm));
    bmd->setScaleFactor(p().geometryscale);
    bmd->setDefaultPatch("walls", "wall");

    double eps=0.01*arma::min(sp().bb_.col(1)-sp().bb_.col(0));
    std::map<int, bmd::Point> pt = boost::assign::map_list_of
                                   (0, 	vec3(sp().bb_(0,0)-eps, sp().bb_(1,0)-eps, sp().bb_(2,0)-eps))
                                   (1, 	vec3(sp().bb_(0,1)+eps, sp().bb_(1,0)-eps, sp().bb_(2,0)-eps))
                                   (2, 	vec3(sp().bb_(0,1)+eps, sp().bb_(1,1)+eps, sp().bb_(2,0)-eps))
                                   (3, 	vec3(sp().bb_(0,0)-eps, sp().bb_(1,1)+eps, sp().bb_(2,0)-eps))
                                   (4, 	vec3(sp().bb_(0,0)-eps, sp().bb_(1,0)-eps, sp().bb_(2,1)+eps))
                                   (5, 	vec3(sp().bb_(0,1)+eps, sp().bb_(1,0)-eps, sp().bb_(2,1)+eps))
                                   (6, 	vec3(sp().bb_(0,1)+eps, sp().bb_(1,1)+eps, sp().bb_(2,1)+eps))
                                   (7, 	vec3(sp().bb_(0,0)-eps, sp().bb_(1,1)+eps, sp().bb_(2,1)+eps))
                                       .convert_to_container<std::map<int, bmd::Point> >()
        ;

    // create patches
    {
        bmd->addBlock
            (
                new Block(
                    {
                        pt[0], pt[1], pt[2], pt[3],
                        pt[4], pt[5], pt[6], pt[7]
                    },
                    sp().nx_, sp().ny_, sp().nz_
                    )
                );
    }
    int nb=bmd->nBlocks();
    cm.insert(bmd.release());
    cm.createOnDisk(executionPath());
    cm.runBlockMesh(executionPath(), nb, &pp);



    create_directory(sp().stldir_);

    snappyHexMeshConfiguration::Parameters shm_cfg;

    for (const auto&w: p().geometry)
    {
        int minLevel = w.second.lm>=0?w.second.lm:p().mesh.minLevel;
        int maxLevel = w.second.lx>=0?w.second.lx:p().mesh.maxLevel;

        if (auto *ref = boost::get<Parameters::geometry_default_type::role_refinementOnly_type>(
                &w.second.role))
        {
            shm_cfg.features.push_back(
                std::make_shared<snappyHexMeshFeats::RefinementGeometry>(
                    snappyHexMeshFeats::RefinementGeometry::Parameters()
                        .set_scalefactor(p().geometryscale)
                        .set_geometry(w.second.file)
                        .set_dist(ref->dist)
                        .set_mode(
                            static_cast<snappyHexMeshFeats::RefinementGeometry::Parameters::mode_type>(
                                ref->mode.value))
                        .set_level(maxLevel)
                        .set_name(w.first)
                    ));
        }
        else if (auto *vol = boost::get<Parameters::geometry_default_type::role_porousVolume_type>(
                     &w.second.role))
        {
            shm_cfg.features.push_back(
                std::make_shared<snappyHexMeshFeats::RefinementGeometry>(
                    snappyHexMeshFeats::RefinementGeometry::Parameters()
                        .set_scalefactor(p().geometryscale)
                        .set_geometry(w.second.file)
                        .set_level(maxLevel)
                        .set_name(w.first)
                    ));
        }
        else
        {
            auto feat=surfaceFeatureExtract(w.second.file->geometry(), 30.);

            shm_cfg.features.push_back(
                snappyHexMeshFeats::FeaturePtr(
                    new snappyHexMeshFeats::ExplicitFeatureCurve(
                        snappyHexMeshFeats::ExplicitFeatureCurve::Parameters()
                            .set_level(maxLevel)
                            .set_scalefactor(p().geometryscale)
                            .set_geometry(make_geometryFile(feat))
                            .set_name(w.first+"_features")
                        )));

            int nLayers=0;
            if (boost::get<Parameters::geometry_default_type::role_wall_type>(&w.second.role))
            {
                nLayers=p().mesh.nLayers;
            }

            shm_cfg.features.push_back(
                snappyHexMeshFeats::FeaturePtr(
                    new snappyHexMeshFeats::Geometry(
                        snappyHexMeshFeats::Geometry::Parameters()
                            .set_minLevel(minLevel)
                            .set_maxLevel(maxLevel)
                            .set_nLayers(nLayers)
                            .set_scalefactor(p().geometryscale)
                            .set_geometry(w.second.file)
                            .set_name(w.first)
                        )));
        }
    }

    shm_cfg.PiM.push_back(p().mesh.PiM);
    shm_cfg.tlayer=p().mesh.tlayer;
    shm_cfg.erlayer=p().mesh.erlayer;
    shm_cfg.relativeSizes=p().mesh.relativeSizes;

    snappyHexMesh
        (
            cm, executionPath(),
            shm_cfg,
            true, false, false,
            &pp
            );


    resetMeshToLatestTimestep(cm, executionPath(), true);

    cm.executeCommand(executionPath(), "renumberMesh", list_of("-overwrite"));

}


void InternalPressureLossBase::applyCustomPreprocessing(OpenFOAMCase& cm, ProgressDisplayer& progress)
{
    boost::filesystem::path STLOutPath =
        snappyHexMeshFeats::geometryDir(cm, this->executionPath());

    std::vector<std::string> cellSetCmds;

    for (const auto&w: p().geometry)
    {
        if (auto *vol = boost::get<Parameters::geometry_default_type::role_porousVolume_type>(
                &w.second.role))
        {

            auto stl=STLOutPath/(w.first+".stlb");
            cellSetCmds.push_back(
                "cellSet "+w.first+
                " new surfaceToCell \""
                +stl.string()
                +"\" ("
                +OFDictData::toString(
                    OFDictData::vector3(
                        p().mesh.PiM))
                +") 0 1 0 0 0"
                );
        }
    }

    if (cellSetCmds.size())
    {
        setSet(cm, executionPath(), cellSetCmds);
        cm.executeCommand(executionPath(), "setsToZones", { "-noFlipMap" } );
    }
}


std::unique_ptr<OpenFOAMCaseElement>
InternalPressureLossBase::createWallBC(
    insight::OpenFOAMCase& cm, const std::string& patchName, const OFDictData::dict& boundaryDict) const
{
    return std::make_unique<WallBC>(
        cm, patchName, boundaryDict,
        WallBC::Parameters() );
}

double InternalPressureLossBase::inletTemperature(const std::string &patchName) const
{
    return 300.0;
}

std::unique_ptr<OpenFOAMCaseElement>
InternalPressureLossBase::createInletBC(
    OpenFOAMCase &cm,
    const std::string &patchName,
    const Parameters::geometry_default_type::role_inlet_type* in,
    const OFDictData::dict &boundaryDict) const
{
    // grid needs to be present
    patchArea inletprops(
        OpenFOAMCase(OFEs::get(p().run.OFEname)),
        executionPath(), patchName);
    double D=sqrt(inletprops.A_*4./M_PI);
    double turbI=0.1;
    double turbL=D*0.2;

    auto turb = std::make_shared<turbulenceBC::uniformIntensityAndLengthScale>(
        turbulenceBC::uniformIntensityAndLengthScale::Parameters()
            .set_I(turbI) .set_l(turbL)
        );

    if (auto* mfl=boost::get<Parameters::geometry_default_type::role_inlet_type
                               ::specification_massFlow_type>(
            &in->specification))
    {
        MassflowBC::Parameters inp;
        MassflowBC::Parameters::flowrate_massflow_type mf = { mfl->dotm };
#warning check handling of rho for incompressible case
        inp.flowrate = mf;
        inp.turbulence=turb;
        inp.temperature=MassflowBC::Parameters::temperature_staticTemperature_type{
            inletTemperature(patchName) };
        return std::make_unique<MassflowBC>(cm, patchName, boundaryDict, inp);
    }
    else if (auto* vfl=boost::get<Parameters::geometry_default_type::role_inlet_type
                                    ::specification_volumetricFlow_type>(
                 &in->specification))
    {
        MassflowBC::Parameters inp;
        MassflowBC::Parameters::flowrate_volumetric_type mf = { vfl->Q };
        inp.flowrate = mf;
        inp.turbulence=turb;
        inp.temperature=MassflowBC::Parameters::temperature_staticTemperature_type{
            inletTemperature(patchName) };
        return std::make_unique<MassflowBC>(cm, patchName, boundaryDict, inp);
    }
    else if (auto* vfl=boost::get<Parameters::geometry_default_type::role_inlet_type::specification_vector_type>(
                 &in->specification))
    {
        VelocityInletBC::Parameters inp;
        inp.velocity=FieldData::uniformSteady(vfl->velocity);
        inp.turbulence=turb;
        inp.T=FieldData::uniformSteady(inletTemperature(patchName));
        return std::make_unique<VelocityInletBC>(cm, patchName, boundaryDict, inp);
    }
    else if (auto* suc=boost::get<Parameters::geometry_default_type::role_inlet_type
                                    ::specification_pressureInlet_type>(
                 &in->specification))
    {
        SuctionInletBC::Parameters inp;
        inp.turb_I=turbI;
        inp.turb_L=turbL;
        inp.pressure=suc->ambientPressure;
        inp.T=inletTemperature(patchName);
        return std::make_unique<SuctionInletBC>(cm, patchName, boundaryDict, inp);
    }
    else
        throw insight::UnhandledSelection();

    return nullptr;
}

std::unique_ptr<calculateTotalPressure::Parameters>
InternalPressureLossBase::totalPressureCalculationParameters() const
{
    auto ctp=std::make_unique<calculateTotalPressure::Parameters>();
    (*ctp)
        .set_pName("p")
        .set_UName("U")
        .set_name("calcTotalPressure")
        ;
    return ctp;
}

double InternalPressureLossBase::ambientPressure() const
{
    return 0;
}



void InternalPressureLossBase::createCase(insight::OpenFOAMCase& cm, ProgressDisplayer&)
{


    OFDictData::dict boundaryDict;
    cm.parseBoundaryDict(executionPath(), boundaryDict);

    auto num = cm.insert(numericsCaseElement(cm));


    /***********************************************************************************************
     * boundary conditions
     ***********************************************************************************************/

    for (auto& g: p().geometry)
    {

        /****
         * inlet
         ****/
        if (auto *in =boost::get<Parameters::geometry_default_type::role_inlet_type>(
                &g.second.role))
        {
            cm.insert(createInletBC(cm, g.first, in, boundaryDict));
        }
        /*****
         * outlet
         *****/
        else if (auto *out =boost::get<Parameters::geometry_default_type::role_outlet_type>(
                     &g.second.role))
        {
            cm.insert(new PressureOutletBC(
                cm, g.first, boundaryDict, PressureOutletBC::Parameters()
                    .set_behaviour( PressureOutletBC::Parameters::behaviour_uniform_type(
                        FieldData::Parameters()
                            .set_fielddata(FieldData::Parameters::fielddata_uniformSteady_type(
                                vec1(ambientPressure()+out->pressure))), false
                        ))
                ));

        }
        /*****
         * wall
         *****/
        else if (auto *wall =boost::get<Parameters::geometry_default_type::role_wall_type>(
                     &g.second.role))
        {
            cm.insert(createWallBC(cm, g.first, boundaryDict));
        }
        /*****
         * symmetry
         *****/
        else if (auto *symm =boost::get<Parameters::geometry_default_type::role_symmetry_type>(
                     &g.second.role))
        {
            cm.insert(new SymmetryBC(cm, g.first, boundaryDict));
        }
        /*****
         * porous zone
         *****/
        else if (auto *vol = boost::get<Parameters::geometry_default_type::role_porousVolume_type>(
                     &g.second.role))
        {
            cm.insert(new porousZoneOption(
                cm, *porousZoneParameters(g.first, vol)));
        }
    }


    cm.insert(new calculateTotalPressure( cm, *totalPressureCalculationParameters() ));

    for (auto& g: p().geometry)
    {

        /****
         * inlet or outlet
         ****/
        if (
            (boost::get<Parameters::geometry_default_type::role_inlet_type>(
                &g.second.role))
            ||
            (boost::get<Parameters::geometry_default_type::role_outlet_type>(
                &g.second.role))
            )
        {
            cm.insert(new surfaceIntegrate(
                cm, surfaceIntegrate::Parameters()
                    .set_domain( surfaceIntegrate::Parameters::domain_patch_type( g.first ) )
                    .set_fields( { "pTotal", "p", "T" } )
                    .set_operation( surfaceIntegrate::Parameters::areaAverage )
                    .set_outputControl("timeStep")
                    .set_outputInterval(1)
                    .set_name(g.first+"_pressure")
                ));
        }
    }


}





} // namespace insight
