#include "base/cppextensions.h"
#include "internalpressureloss.h"

#include "openfoam/caseelements/numerics/steadyincompressiblenumerics.h"
#include "openfoam/caseelements/numerics/steadycompressiblenumerics.h"
#include "openfoam/caseelements/analysiscaseelements.h"


#include "base/vtkrendering.h"
#include "vtkPointSource.h"
#include "vtkStreamTracer.h"
#include "vtkDataSetMapper.h"
#include "vtkPolyDataMapper.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkMaskPoints.h"
#include "vtkCutter.h"
#include "vtkPlane.h"
#include "vtkIntegrateAttributes.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPolyDataNormals.h"
#include "vtkArrayCalculator.h"
#include <memory>
#include <string>

namespace insight
{


std::pair<std::set<std::string>, std::set<std::string> >
InternalPressureLossBase::findInOutPatches() const
{
    std::set<std::string> inletPatches;
    std::set<std::string> outletPatches;
    for (auto& g: p().geometry)
    {
        /****
             * inlet or outlet
             ****/
        if (auto *in=boost::get<Parameters::geometry_default_type::role_inlet_type>(
                &g.second.role))
        {

            if (auto *mf =
                boost::get<Parameters::geometry_default_type::role_inlet_type::specification_massFlow_type>(
                    &in->specification))
            {
                if (mf->dotm>0.)
                    inletPatches.insert(g.first);
                else
                    outletPatches.insert(g.first);
            }
            else if (auto *vf =
                     boost::get<Parameters::geometry_default_type::role_inlet_type::specification_volumetricFlow_type>(
                         &in->specification))
            {
                if (vf->Q>0.)
                    inletPatches.insert(g.first);
                else
                    outletPatches.insert(g.first);
            }
            else
                inletPatches.insert(g.first);
        }
        else if (auto *out=boost::get<Parameters::geometry_default_type::role_outlet_type>(
                     &g.second.role))
        {
            outletPatches.insert(g.first);
        }
    }
    return {inletPatches, outletPatches};
};


InternalPressureLoss::RenderingScene::RenderingScene(
    const boost::filesystem::path& exePath,
    const OFDictData::dict& boundaryDict,
    InOutPatches inOutPatches,
    std::function<const Parameters&(void)> p,
    std::function<const supplementedInputData&(void)> sp
)
  : scene( exePath/"system"/"controlDict" ),
    in_out(inOutPatches)
{
    inlet = scene.patchesFilter("("+boost::join(in_out.first, "|")+")");
    outlet = scene.patchesFilter("("+boost::join(in_out.second, "|")+")");

    for (const auto&p: boundaryDict)
    {
        auto name = p.first;
        auto patchDict = boost::get<OFDictData::dict>(p.second);
        if (patchDict.getString("type")=="wall")
        {
            wallPatchNames.push_back(name);
        }
    }
    patches = scene.patchesFilter(
        "("+boost::join(wallPatchNames, "|")+")" );
    internal = scene.internalMeshFilter();


    // use domain center instead of input geometry center,
    // since much of the input geometry might not have been
    // included in the actual domain
    internal->Update();
    arma::mat bb = PolyDataBndBox(internal->GetOutput());


    L=p().geometryscale*sp().L_;
    Lmax=p().geometryscale*arma::as_scalar(arma::max(sp().L_));

    objCS=CoordinateSystem(
        ( bb.col(1) + bb.col(0) )*0.5,
        vec3X(), vec3Z() );

    views = generateStandardViews(
        objCS,
        Lmax );

    camera = scene.activeCamera();
    camera->ParallelProjectionOn();

}

ResultSetPtr InternalPressureLossBase::evaluateResults(OpenFOAMCase& cm, ProgressDisplayer& pp)
{
    auto& numerics = cm.getUniqueElement<FVNumerics>();

    auto results=insight::OpenFOAMAnalysis::evaluateResults(cm, pp);

    auto ap = pp.forkNewAction(8, "Evaluation");


    ap->message("Computing average surface pressure...");
    {
        std::map<std::string, double> inletPatches, outletPatches;

        auto in_out=findInOutPatches();
        for (auto& g: p().geometry)
        {
            decltype(inletPatches)* pl{nullptr};

            /****
             * inlet or outlet
             ****/

            if (in_out.first.count(g.first))
                pl=&inletPatches;
            else if (in_out.second.count(g.first))
                pl=&outletPatches;
            // else wall or so


            if (pl) // if inlet or outlet
            {
                auto sec = std::make_unique<ResultSection>("Total pressure on boundary "+g.first);

                std::vector<std::string> colNames;
                arma::mat ptot_vs_t_raw =
                    surfaceIntegrate::readSurfaceIntegrate(
                        cm, executionPath(), g.first+"_pressure", "", &colNames );

                auto it = std::find(colNames.begin(), colNames.end(), "areaAverage(pTotal)");
                insight::assertion(
                    it!=colNames.end(),
                    "area average of pTotal for boundary %s not found in postprocessing data",
                    g.first.c_str());
                int i=std::distance(colNames.begin(), it);

                arma::mat ptot_vs_t = movingAverage ( ptot_vs_t_raw, p().eval.averageFraction );

                PlotCurve pc(
                    ptot_vs_t.col(0), ptot_vs_t.col(i),
                    "ptotmean_vs_iter", "w l t 'moving average'");
                auto mima=pc.significantMinMax();

                addPlot
                    (
                        *sec, executionPath(), "chartPressureDifference",
                        "Iteration", "$p_{total}=p+\\frac 1 2 \\rho |\\vec u|^2$",
                        {
                            PlotCurve(
                                ptot_vs_t_raw.col(0), ptot_vs_t_raw.col(i),
                                "ptot_raw_vs_iter", "w l t 'raw value'"),
                            pc
                        },
                        "Plot of total pressure on boundary "+g.first,
                        str ( format ( "set yrange [%g:%g]" )
                            % mima.first % mima.second )
                        );

                double finalValue=ptot_vs_t(ptot_vs_t.n_rows-1, 1);



                (*pl)[g.first]=finalValue;

                sec->insert<ScalarResult>(
                    "totalPressure",
                    finalValue, "area-averaged total pressure on boundary "+g.first, "", "Pa");

                results->insert("totalPressureBoundary"+g.first, std::move(sec));
            }
        }


        for (auto& in: inletPatches)
        {
            for (auto& out: outletPatches)
            {
                if (in.first!=out.first)
                {
                    double delta_p = in.second - out.second;

                    results->insert<ScalarResult>(
                        "deltaP_"+in.first+"_"+out.first,
                        delta_p,
                        "Pressure difference between inlet "+in.first+" and outlet "+out.first,
                        "", "Pa");
                }
            }
        }

    }
    ++*ap;


    /*
     * evaluate mass flows through in/outlets
     */
    {
        ap->message("Mass flows...");
        auto sec = std::make_unique<ResultSection>(
            "Mass flows" );

        for (auto&g: p().geometry)
        {
            auto patchName=g.first;
            if (boost::get<Parameters::geometry_default_type::role_inlet_type>(
                    &g.second.role)
                ||
                boost::get<Parameters::geometry_default_type::role_outlet_type>(
                    &g.second.role) )
            {

                arma::mat dotm_vs_t_raw =
                    surfaceIntegrate::readSurfaceIntegrate(
                        cm, executionPath(), "integ_"+patchName );

                dotm_vs_t_raw.col(1)=
                    dotm_vs_t_raw.col(1)*solverProperties().phiFactor;

                arma::mat dotm_vs_t =
                    movingAverage ( dotm_vs_t_raw, p().eval.averageFraction );

                PlotCurve pc(
                    dotm_vs_t.col(0), dotm_vs_t.col(1),
                    "massflow_vs_iter", "w l t 'moving average'");
                auto mima=pc.significantMinMax();

                addPlot
                    (
                        *sec, executionPath(), "chartMassFlow_"+patchName,
                        "Iteration", "$\\dot{m}$",
                        {
                            PlotCurve(
                                dotm_vs_t_raw.col(0), dotm_vs_t_raw.col(1),
                                "massflow_raw_vs_iter", "t 'raw value' w l" + std::string(dotm_vs_t_raw.n_rows==1?"p":"") ),
                            pc
                        },
                        "Plot of mass flow on boundary "+patchName,
                        str ( format ( "set yrange [%g:%g]" )
                            % mima.first % mima.second )
                        );

                double finalValue=dotm_vs_t(dotm_vs_t.n_rows-1, 1);

                sec->insert<ScalarResult>(
                    "massFlow_"+patchName,
                    finalValue, "mass flow through boundary "+patchName, "", "kg/s");
                sec->insert<ScalarResult>(
                    "massFlow_"+patchName+"_kgPerHr",
                    finalValue*3600., "mass flow through boundary "+patchName, "", "kg/hr");

            }
        }
        results->insert("massFlows", std::move(sec));
        ++*ap;
    }


    ap->message("Rendering images...");
    {
        RenderingScene rs(
            executionPath(),
            *cm.parseBoundaryDict(executionPath()),
            findInOutPatches(),
            std::bind(&InternalPressureLossBase::p, this),
            std::bind(&InternalPressureLossBase::sp, this) );
        auto&scene=rs.scene;

        FieldSelection p_field("p", FieldSupport::OnPoint, -1);
        FieldColor p_fc(p_field, createColorMap(), calcRange(p_field, {}, {rs.patches}));

        FieldSelection U_field("U", FieldSupport::OnPoint, -1);
        FieldColor U_fc(U_field, createColorMap(), calcRange(U_field, {}, {rs.internal}));

        // // A renderer and render window
        // OpenFOAMCaseScene scene( executionPath()/"system"/"controlDict" );

        // auto in_out=findInOutPatches();
        // auto inlet = scene.patchesFilter("("+boost::join(in_out.first, "|")+")");
        // auto outlet = scene.patchesFilter("("+boost::join(in_out.second, "|")+")");
        // std::vector<std::string> wallPatchNames;
        // for (const auto&p: boundaryDict)
        // {
        //     auto name = p.first;
        //     auto patchDict = boost::get<OFDictData::dict>(p.second);
        //     if (patchDict.getString("type")=="wall")
        //     {
        //         wallPatchNames.push_back(name);
        //     }
        // }
        // auto patches = scene.patchesFilter(
        //     "("+boost::join(wallPatchNames, "|")+")" );
        // auto internal = scene.internalMeshFilter();


        // // use domain center instead of input geometry center,
        // // since much of the input geometry might not have been
        // // included in the actual domain
        // internal->Update();
        // arma::mat bb = PolyDataBndBox(internal->GetOutput());

        // FieldSelection p_field("p", FieldSupport::OnPoint, -1);
        // FieldColor p_fc(p_field, createColorMap(), calcRange(p_field, {}, {patches}));

        // FieldSelection U_field("U", FieldSupport::OnPoint, -1);
        // FieldColor U_fc(U_field, createColorMap(), calcRange(U_field, {}, {internal}));

        // arma::mat L=p().geometryscale*sp().L_;
        // double Lmax=p().geometryscale*arma::as_scalar(arma::max(sp().L_));

        // CoordinateSystem objCS(
        //     ( bb.col(1) + bb.col(0) )*0.5,
        //     vec3X(), vec3Z() );

        // auto views = generateStandardViews(
        //     objCS,
        //     Lmax );

        // auto camera = scene.activeCamera();
        // camera->ParallelProjectionOn();



        // display streamtracers, colored by velocity
        {
            auto seeds = vtkSmartPointer<vtkMaskPoints>::New();
            seeds->SetInputConnection(rs.inlet->GetOutputPort());
            seeds->SetMaximumNumberOfPoints(100);
            seeds->SetRandomMode(true);
            seeds->SetRandomModeType(1);

            auto st = vtkSmartPointer<vtkStreamTracer>::New();
            st->SetInputConnection(rs.internal->GetOutputPort());
            st->SetSourceConnection(seeds->GetOutputPort());
            st->SetMaximumPropagation(10.*rs.Lmax);
            st->SetIntegrationDirectionToBoth();
            st->SetInputArrayToProcess(
                0, 0, 0,
                vtkDataObject::FIELD_ASSOCIATION_POINTS,
                "U");

            scene.add(st->GetOutputPort(), U_fc);
        }

        // display walls, transparent, gray color
        auto pa = scene.add(rs.patches->GetOutputPort(), vec3(0.7, 0.7, 0.7));
        pa->GetProperty()->SetOpacity(0.1);

        auto sec_sl = std::make_unique<ResultSection>("Streamlines");
        for (const auto& lv: rs.views)
        {
            scene.setupActiveCamera(lv.second);
            scene.fitAll();

            auto img = executionPath() / ("streamLines_"+lv.first+".png");

            sec_sl->insert(img.filename().stem().string(),
                           std::unique_ptr<Image>(new Image
                                                  (
                                                      FileContainer(*scene.exportImage(), img.filename()),
                                                      "Stream lines ("+lv.second.title+")", ""
                                                      )));

            ++*ap;
        }
        results->insert("streamlines", std::move(sec_sl));




        scene.clearScene();

        scene.add(rs.patches->GetOutputPort(), p_fc);
        scene.addColorBar(
            numerics.isCompressible() ? "Pressure\n[Pa]" : "Pressure\n[m^2/s^2]",
            p_fc.lookupTable());

        auto sec_pres = std::make_unique<ResultSection>("Pressure on walls");
        for (const auto& lv: rs.views)
        {
            scene.setupActiveCamera(lv.second);
            scene.fitAll();

            auto img = executionPath() / ("pressure_"+lv.first+".png");

            sec_pres->insert(img.filename().stem().string(),
                             std::unique_ptr<Image>(new Image
                                                    (
                                                        FileContainer(*scene.exportImage(), img.filename()),
                                                        "Pressure on walls ("+lv.second.title+")", ""
                                                        )));

            ++*ap;
        }
        results->insert("pressure_walls", std::move(sec_pres));


        auto cpl = std::container_type_cast<std::map<std::string,arma::mat> >(
            p().eval.additionalCutPlaneLocations);
        cpl.emplace("center", rs.objCS.origin);

        for (auto& l: cpl)
        {
            auto sec=std::make_unique<ResultSection>("Cut Plane "+l.first);

            auto cutplane1 = vtkSmartPointer<vtkCutter>::New();
            auto cutplane3 = vtkSmartPointer<vtkCutter>::New();
            auto cutplane2 = vtkSmartPointer<vtkCutter>::New();

            {
                cutplane1->SetInputConnection(rs.internal->GetOutputPort());

                auto slpl = vtkSmartPointer<vtkPlane>::New();
                slpl->SetOrigin(toArray(l.second));
                slpl->SetNormal(toArray(rs.objCS.ex));
                cutplane1->SetCutFunction(slpl);
            }
            {
                cutplane2->SetInputConnection(rs.internal->GetOutputPort());

                auto slpl = vtkSmartPointer<vtkPlane>::New();
                slpl->SetOrigin(toArray(l.second));
                slpl->SetNormal(toArray(rs.objCS.ey));
                cutplane2->SetCutFunction(slpl);
            }
            {
                cutplane3->SetInputConnection(rs.internal->GetOutputPort());

                auto slpl = vtkSmartPointer<vtkPlane>::New();
                slpl->SetOrigin(toArray(l.second));
                slpl->SetNormal(toArray(rs.objCS.ez));
                cutplane3->SetCutFunction(slpl);
            }

            scene.clearScene();

            scene.add(cutplane1->GetOutputPort(), U_fc);
            scene.add(cutplane2->GetOutputPort(), U_fc);
            scene.add(cutplane3->GetOutputPort(), U_fc);
            scene.addColorBar("Velocity\n[m/s]", U_fc.lookupTable());


            auto sec_u = std::make_unique<ResultSection>("Velocity in cut planes");
            for (const auto& lv: rs.views)
            {
                scene.setupActiveCamera(lv.second);
                scene.fitAll();

                auto img = executionPath() / ("velocity_cut_"+lv.first+".png");

                sec_u->insert(
                    img.filename().stem().string(),
                    std::make_unique<Image>(
                        FileContainer(*scene.exportImage(), img.filename()),
                        "Velocity in cut planes ("+lv.second.title+")", ""
                        ));

                ++*ap;
            }
            sec->insert("velocity_cutplanes", std::move(sec_u));


            scene.clearScene();

            scene.add(cutplane1->GetOutputPort(), p_fc);
            scene.add(cutplane2->GetOutputPort(), p_fc);
            scene.add(cutplane3->GetOutputPort(), p_fc);
            scene.addColorBar("Pressure\n[m^2/s^2]", p_fc.lookupTable());

            auto sec_pc = std::make_unique<ResultSection>("Pressure in cut planes");
            for (const auto& lv: rs.views)
            {
                scene.setupActiveCamera(lv.second);
                scene.fitAll();

                auto img = executionPath() / ("pressure_cut_"+lv.first+".png");
                sec_pc->insert(
                    img.filename().stem().string(),
                    std::make_unique<Image>(
                        FileContainer(*scene.exportImage(), img.filename()),
                        "Pressure in cut planes ("+lv.second.title+")", ""
                        ));

                ++*ap;
            }
            sec->insert("pressure_cutplanes", std::move(sec_pc));

            results->insert("cutPlane_"+l.first, std::move(sec));
        }

    }


    return results;
}



ResultSetPtr InternalPressureLoss::evaluateResults(
    OpenFOAMCase& cm, ProgressDisplayer& pp )
{
    auto ap = pp.forkNewAction(8, "Extended Evaluation");

    auto results=InternalPressureLossBase::evaluateResults(cm, pp);

    if (const auto* thermsolve =
        boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
            &p().operation.thermalTreatment))
    {
        ap->message("Computing average outlet temperature...");
        arma::mat T_vs_t = surfaceIntegrate::readSurfaceIntegrate(cm, executionPath(), "outlet_temperature");
        ++*ap;

        arma::mat Tsig = T_vs_t.rows(T_vs_t.n_rows/3, T_vs_t.n_rows-1).col(1);

        ap->message("Producing temperature convergence history plot...");
        addPlot
            (
                *results, executionPath(), "chartTemperature",
                "Iteration", "$T/K$",
                {
                    PlotCurve(T_vs_t.col(0), T_vs_t.col(1), "Tmean_vs_iter", "w l not")
                },
                "Plot of temperature, spatially averaged over outlet, vs. iterations",
                str ( format ( "set yrange [%g:%g]" )
                    % ( std::min ( 0.0, 1.1*Tsig.min() ) )
                    % ( 1.1*Tsig.max() ) )
                );
        ++*ap;

        double Tfinal=T_vs_t(T_vs_t.n_rows-1,1);
        results->insert<ScalarResult>("Tfinal", Tfinal, "Temperature in outlet", "", "K");
    }


    RenderingScene rs(
        executionPath(),
        *cm.parseBoundaryDict(executionPath()),
        findInOutPatches(),
        std::bind(&InternalPressureLossBase::p, this),
        std::bind(&InternalPressureLossBase::sp, this) );
    auto&scene=rs.scene;

    auto cpl = std::container_type_cast<std::map<std::string,arma::mat> >(
        p().eval.additionalCutPlaneLocations);
    cpl.emplace("center", rs.objCS.origin);

    for (auto& l: cpl)
    {
        auto sec=std::make_unique<ResultSection>("Cut Plane "+l.first);

        auto cutplane1 = vtkSmartPointer<vtkCutter>::New();
        auto cutplane3 = vtkSmartPointer<vtkCutter>::New();
        auto cutplane2 = vtkSmartPointer<vtkCutter>::New();

        if (const auto* thermsolve =
            boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
                &p().operation.thermalTreatment))
        {


            FieldSelection T_field("T", FieldSupport::OnPoint, -1);
            FieldColor T_fc(T_field,
                            createColorMap(colorMapData_CoolToWarm, 32, true),
                            calcRange(T_field, {}, {rs.internal})
                            );

            scene.clearScene();

            scene.add(cutplane1->GetOutputPort(), T_fc);
            scene.add(cutplane2->GetOutputPort(), T_fc);
            scene.add(cutplane3->GetOutputPort(), T_fc);
            scene.addColorBar("Temperature\n[K]", T_fc.lookupTable());


            auto sec_T = std::make_unique<ResultSection>("Temperature in cut planes");
            for (const auto& lv: rs.views)
            {
                scene.setupActiveCamera(lv.second);
                scene.fitAll();

                auto img = executionPath() / ("temperature_cut_"+lv.first+".png");
                sec_T->insert(
                    img.filename().stem().string(),
                    std::make_unique<Image>(
                        FileContainer(*scene.exportImage(), img.filename()),
                        "Temperature in cut planes ("+lv.second.title+")", ""
                        ));

                ++*ap;
            }
            sec->insert("temperature_cutplanes", std::move(sec_T));

            scene.clearScene();
        }

        results->insert("cutPlane_"+l.first, std::move(sec));
    }


    if (const auto* thermsolve =
        boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
            &p().operation.thermalTreatment))
    {


        FieldSelection T_field("T", FieldSupport::OnPoint, -1);
        FieldColor T_fc(T_field,
                        createColorMap(colorMapData_CoolToWarm, 32, true),
                        calcRange(T_field, {}, {rs.internal})
                        );

        // display streamtracers, colored by temperature
        {
            auto seeds = vtkSmartPointer<vtkMaskPoints>::New();
            seeds->SetInputConnection(rs.inlet->GetOutputPort());
            seeds->SetMaximumNumberOfPoints(100);
            seeds->SetRandomMode(true);
            seeds->SetRandomModeType(1);

            auto st = vtkSmartPointer<vtkStreamTracer>::New();
            st->SetInputConnection(rs.internal->GetOutputPort());
            st->SetSourceConnection(seeds->GetOutputPort());
            st->SetMaximumPropagation(10.*rs.Lmax);
            st->SetIntegrationDirectionToBoth();
            st->SetInputArrayToProcess(
                0, 0, 0,
                vtkDataObject::FIELD_ASSOCIATION_POINTS,
                "U");

            // display streamlines
            scene.add(st->GetOutputPort(), T_fc);
        }

        // display walls, transparent, gray color
        auto pa = scene.add(rs.patches->GetOutputPort(), vec3(0.7, 0.7, 0.7));
        pa->GetProperty()->SetOpacity(0.1);

        auto sec_slt = std::make_unique<ResultSection>("Streamlines with Temperature");
        for (const auto& lv: rs.views)
        {
            scene.setupActiveCamera(lv.second);
            scene.fitAll();

            auto img = executionPath() / ("streamLinesTemp_"+lv.first+".png");

            sec_slt->insert(img.filename().stem().string(),
                            std::unique_ptr<Image>(new Image
                                                   (
                                                       FileContainer(*scene.exportImage(), img.filename()),
                                                       "Stream lines with temperature ("+lv.second.title+")", ""
                                                       )));

            ++*ap;
        }
        results->insert("streamlines_temperature", std::move(sec_slt));




        T_fc = FieldColor(T_field,
                          createColorMap(colorMapData_CoolToWarm, 32, true),
                          calcRange(T_field, {}, {rs.outlet})
                          );



        auto sec_To = std::make_unique<ResultSection>("Temperature in outlet");

        forEachUnconnectedPart(
            scene, executionPath(), sec_To.get(), rs.outlet,
            [&](vtkAlgorithm* region, ResultSection* sec, int i )
            {

                scene.add(region->GetOutputPort(), T_fc);
                scene.addColorBar("Temperature\n[K]", T_fc.lookupTable());

                for (const auto& lv: rs.views)
                {
                    if (lv.first=="front" || lv.first=="left" || lv.first=="above")
                    {
                        scene.setupActiveCamera(lv.second);
                        scene.fitAll();

                        auto img = executionPath() /
                                   ("temperature_outlet_"
                                    +lv.first
                                    +(i>=0?"_"+toString(i+1):"")
                                    +".png");

                        sec->insert(
                            img.filename().stem().string(),
                            std::make_unique<Image>(
                                FileContainer(*scene.exportImage(), img.filename()),
                                "Temperature in outlet ("+lv.second.title+")", ""
                                ));

                        ++*ap;
                    }
                }


                // display streamtracers, colored by temperature
                {
                    auto seeds = vtkSmartPointer<vtkMaskPoints>::New();
                    seeds->SetInputConnection(region->GetOutputPort());
                    seeds->SetMaximumNumberOfPoints(100);
                    seeds->SetRandomMode(true);
                    seeds->SetRandomModeType(1);

                    auto st = vtkSmartPointer<vtkStreamTracer>::New();
                    st->SetInputConnection(rs.internal->GetOutputPort());
                    st->SetSourceConnection(seeds->GetOutputPort());
                    st->SetMaximumPropagation(10.*rs.Lmax);
                    st->SetIntegrationDirectionToBoth();
                    st->SetInputArrayToProcess(
                        0, 0, 0,
                        vtkDataObject::FIELD_ASSOCIATION_POINTS,
                        "U");

                    // display walls, transparent, gray color
                    auto pa = scene.add(rs.patches->GetOutputPort(), vec3(0.7, 0.7, 0.7));
                    pa->GetProperty()->SetOpacity(0.1);

                    scene.add(st->GetOutputPort(), T_fc);

                    scene.addColorBar("Temperature\n[K]", T_fc.lookupTable());

                    for (const auto& lv: rs.views)
                    {
                        if (lv.first=="diag1")
                        {
                            scene.setupActiveCamera(lv.second);
                            scene.fitAll();

                            auto img = executionPath() /
                                       ("streamlines_outlet_"
                                        +lv.first
                                        +(i>=0?"_"+toString(i+1):"")
                                        +".png");

                            sec->insert(
                                img.filename().stem().string(),
                                std::make_unique<Image>(
                                    FileContainer(*scene.exportImage(), img.filename()),
                                    "Streamlines from outlet ("+lv.second.title+")", ""
                                    ));

                            ++*ap;
                        }
                    }
                }

                auto es = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                es->SetInputConnection(region->GetOutputPort());

                auto normals=vtkSmartPointer<vtkPolyDataNormals>::New();
                normals->SetInputConnection(es->GetOutputPort());
                normals->ComputeCellNormalsOn();
                normals->SplittingOn();
                normals->ConsistencyOn();

                auto sfCalc=vtkSmartPointer<vtkArrayCalculator>::New();
                sfCalc->SetInputConnection(normals->GetOutputPort());
                sfCalc->SetAttributeTypeToCellData();
                sfCalc->AddVectorVariable("Normals", "Normals");
                sfCalc->AddVectorVariable("U", "U");
                sfCalc->SetFunction("U.Normals");
                sfCalc->SetResultArrayName( "Flux" );
                sfCalc->Update();

                auto integ = vtkSmartPointer<vtkIntegrateAttributes>::New();
                integ->SetInputConnection(sfCalc->GetOutputPort());
                integ->Update();

                double Tmean =
                    integ->GetOutput()->GetCellData()->GetArray("T")->GetTuple1(0)
                    /
                    integ->GetOutput()->GetCellData()->GetArray("Area")->GetTuple1(0);
                sec->insert<ScalarResult>("Tmean",
                                          Tmean, "mean temperature", "", "K");

                double flux = integ->GetOutput()->GetCellData()->GetArray("Flux")->GetTuple1(0);
                sec->insert<ScalarResult>("flux",
                                          flux, "volume flux", "", "m^3/s");
            });

        results->insert("temperature_outlet", std::move(sec_To));
    }

    return results;
}


}
