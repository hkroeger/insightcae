
#include "base/exception.h"
#include "base/spatialtransformation.h"
#include "numericalwindtunnel.h"
#include "openfoam/caseelements/analysiscaseelements.h"

#include "base/vtkrendering.h"
#include "vtkDataSetMapper.h"
#include "vtkStreamTracer.h"
#include "vtkPointSource.h"
#include "vtkLineSource.h"
#include "vtkPolyDataMapper.h"

namespace insight
{



ResultSetPtr NumericalWindtunnel::evaluateResults(OpenFOAMCase& cm, ProgressDisplayer& pp)
{
    auto results=insight::OpenFOAMAnalysis::evaluateResults(cm, pp);

    auto ap = pp.forkNewAction(7, "Evaluation");

    // get full name of car patch (depends on STL file)
    OFDictData::dict boundaryDict;
    cm.parseBoundaryDict(executionPath(), boundaryDict);
    std::string carPatchName;
    for (const OFDictData::dict::value_type& de: boundaryDict)
    {
        if (boost::starts_with(de.first, "object"))
        {
            carPatchName=de.first;
            break;
        }
    }

    ap.message("Computing projected area");
    arma::mat projDir;
    switch (p().eval.referenceAreaProjectionDirection)
    {
        case Parameters::eval_type::frontal: projDir=vec3(1,0,0); break;
        case Parameters::eval_type::lateral: projDir=vec3(0,1,0); break;
        case Parameters::eval_type::vertical: projDir=vec3(0,0,1); break;
        default: throw insight::UnhandledSelection();
    }
    arma::mat Ah=projectedArea(
        cm, executionPath(),
        projDir, {"object"} );

    double A=Ah(Ah.n_rows-1,1);
    ++ap;


    double Re=p().operation.v*sp().Lref_/p().fluid.nu;
    results->insert<ScalarResult>("Re", Re, "Reynolds number", "", "");
    results->insert<ScalarResult>("Afront", A, "Projected frontal area", "", "$m^2$");

    ap.message("Reading forces");
    arma::mat f=forces::readForces(cm, executionPath(), sp().FOname);
    arma::mat t = f.col(0);
    ++ap;

    double mult = p().mesh.longitudinalSymmetry ? 2.0 : 1.0;

    arma::mat Rtot = (f.col(1)+f.col(4)) *mult;
    arma::mat Flat = (f.col(2)+f.col(5)) *mult;
    arma::mat L = (f.col(3)+f.col(6)) *mult;

    results->insert<ScalarResult>("Rtot", Rtot(Rtot.n_rows-1), "Total resistance", "", "N");
    results->insert<ScalarResult>("Flat", Flat(Flat.n_rows-1), "Lateral force", "", "N");
    results->insert<ScalarResult>("L", L(L.n_rows-1), "Lifting force", "", "N");

    double denom=(0.5*p().fluid.rho*pow(p().operation.v,2)*A);

    if (fabs(denom)<SMALL)
    {
        insight::Warning("denominator for force normalization is zero! Skipping calculation of force coefficients");
    }
    else
    {
        double cr=Rtot(Rtot.n_rows-1) / denom;
        results->insert<ScalarResult>("cr", cr, "Resistance coefficient", "", "");

        double cl=L(L.n_rows-1) / denom;
        results->insert<ScalarResult>("cl", cl, "Lifting coefficient", "with respect to projected area and forward velocity", "");

        double cs=Flat(Flat.n_rows-1) / denom;
        results->insert<ScalarResult>("cs", cs, "Lateral forces coefficient", "with respect to projected area and forward velocity", "");

    }

    double Pe=Rtot(Rtot.n_rows-1) * p().operation.v;
    results->insert<ScalarResult>("Pe", Pe, "Effective power $P_e=R_{tot} v$", "", "W");

    ap.message("Creating resistance plot");
    // Resistance convergence
    addPlot
        (
            *results, executionPath(), "chartResistance",
            "Iteration", "F [N]",
            {
                PlotCurve( arma::mat(join_rows(t, Rtot)),  "Rtot", "w l lw 2 t 'Total resistance'"),
                PlotCurve( arma::mat(join_rows(t, Flat)), "Flat", "w l lw 2 t 'Lateral force'"),
                PlotCurve( arma::mat(join_rows(t, L)),    "L", "w l lw 2 t 'Lifting force'")
            },
            "Convergence history of resistance force"
            );
    ++ap;

    ap.message("Rendering images");
    {
        // A renderer and render window
        OpenFOAMCaseScene scene( executionPath()/"system"/"controlDict" );

        auto patches = scene.patches("object.*|floor.*");

        FieldSelection sl_field("p", FieldSupport::OnPoint, -1);
        auto sl_range=calcRange(sl_field, {patches}, {});
        auto sl_cm=createColorMap();
        FieldColor sl_fc(sl_field, sl_cm, sl_range);

        scene.addData<vtkDataSetMapper>(patches, sl_fc);
        scene.addColorBar("Pressure\n[m^2/s^2]", sl_cm);

        auto camera = scene.activeCamera();
        camera->ParallelProjectionOn();

        insight::CoordinateSystem evalCS(
            vec3(0.5*sp().l_, 0, sp().Ldown_+0.5*sp().h_),
            vec3X(), vec3Z() );
        double maxObjSize=std::max(sp().l_, std::max(sp().w_, sp().h_));
        double camDist=10.*maxObjSize;

      //   scene.addDiagonalViews(
      //       *results,
      //       evalCS, camDist,
      //       executionPath() / "pressureContour.png",
      //       "Pressure contour on object",
      //       VTKOffscreenScene::ParallelScale(maxObjSize) );
      // ++ap;

        // auto viewctr=vec3(0.5*sp().l_, 0, sp().Ldown_+0.5*sp().h_);
        // camera->SetFocalPoint( toArray(viewctr) );

        // {
        //     camera->SetViewUp( toArray(vec3(0,0,1)) );
        //     camera->SetPosition( toArray(viewctr+vec3(-10.0*sp().l_,0,0)) );

        //     auto img = executionPath() / "pressureContour_front.png";
        //     //      scene.fitAll();
        //     scene.setParallelScale(std::pair<double,double>(sp().w_, sp().h_));
        //     scene.exportImage(img);
        //     results->insert(img.filename().stem().string(),
        //                     std::unique_ptr<Image>(new Image
        //                                            (
        //                                                executionPath(), img.filename(),
        //                                                "Pressure contour (front view)", ""
        //                                                )));
        // }
        // ++ap;

        // {
        //     camera->SetViewUp( toArray(vec3(0,0,1)) );
        //     camera->SetPosition( toArray(viewctr+vec3(0,-10.0*sp().w_,0)) );

        //     auto img = executionPath() / "pressureContour_side.png";
        //     //      scene.fitAll();
        //     scene.setParallelScale(std::pair<double,double>(sp().l_, sp().h_));
        //     scene.exportImage(img);
        //     results->insert(img.filename().stem().string(),
        //                     std::unique_ptr<Image>(new Image
        //                                            (
        //                                                executionPath(), img.filename(),
        //                                                "Pressure contour (side view)", ""
        //                                                )));
        // }
        // ++ap;

        // {
        //     camera->SetViewUp( toArray(vec3(0,1,0)) );
        //     camera->SetPosition( toArray(viewctr+vec3(0,0,10.0*sp().h_)) );

        //     auto img = executionPath() / "pressureContour_top.png";
        //     //      scene.fitAll();
        //     scene.setParallelScale(std::pair<double,double>(sp().l_, sp().w_));
        //     scene.exportImage(img);
        //     results->insert(img.filename().stem().string(),
        //                     std::unique_ptr<Image>(new Image
        //                                            (
        //                                                executionPath(), img.filename(),
        //                                                "Pressure contour (top view)", ""
        //                                                )));
        // }
        // ++ap;

        // {
        //     camera->SetViewUp( toArray(vec3(0,0,1)) );
        //     camera->SetPosition( toArray(viewctr+10.*vec3(-sp().l_,-sp().w_,sp().h_)) );

        //     auto img = executionPath() / "pressureContour_diag.png";
        //     //      scene.fitAll();
        //     double f=sqrt(2.);
        //     scene.setParallelScale(std::pair<double,double>(
        //         std::max(f*sp().l_, f*sp().w_),
        //         std::max(f*sp().l_, f*sp().h_)
        //         ));
        //     scene.exportImage(img);
        //     results->insert(img.filename().stem().string(),
        //                     std::unique_ptr<Image>(new Image
        //                                            (
        //                                                executionPath(), img.filename(),
        //                                                "Pressure contour (isometric view)", ""
        //                                                )));
        // }
        // ++ap;

        // {
        //     camera->SetViewUp( toArray(vec3(0,0,1)) );
        //     camera->SetPosition( toArray(viewctr+10.*vec3(2.*sp().l_,-sp().w_,sp().h_)) );

        //     auto img = executionPath() / "pressureContour_diagRear.png";
        //     //      scene.fitAll();
        //     double f=sqrt(2.);
        //     scene.setParallelScale(std::pair<double,double>(
        //         std::max(f*sp().l_, f*sp().w_),
        //         std::max(f*sp().l_, f*sp().h_)
        //         ));
        //     scene.exportImage(img);
        //     results->insert(img.filename().stem().string(),
        //                     std::unique_ptr<Image>(new Image
        //                                            (
        //                                                executionPath(), img.filename(),
        //                                                "Pressure contour (isometric view from rear)", ""
        //                                                )));
        // }
        // ++ap;

        auto im = scene.internalMesh();

        double Hs=1.25*sp().h_;
        int ns=50;
        {
            // auto seeds = vtkSmartPointer<vtkPointSource>::New();
            // seeds->SetCenter(toArray(vec3(0.5*sp().l_, 0.5*sp().w_, sp().Ldown_+0.5*sp().h_)));
            // seeds->SetRadius(0.2*sp().Lref_);
            // seeds->SetDistributionToUniform();
            // seeds->SetNumberOfPoints(100);
            auto seeds = vtkSmartPointer<vtkLineSource>::New();
            seeds->SetPoint1(toArray(vec3(-0.1*sp().Lupstream_, 0.05*sp().w_, sp().Ldown_)));
            seeds->SetPoint2(toArray(vec3(-0.1*sp().Lupstream_, 0.05*sp().w_, sp().Ldown_+Hs)));
            seeds->SetResolution(ns);

            auto st = vtkSmartPointer<vtkStreamTracer>::New();
            st->SetInputData(im);
            st->SetSourceConnection(seeds->GetOutputPort());
            st->SetIntegrationDirectionToBoth();
            st->SetMaximumPropagation(10.*sp().Ldownstream_);
            st->SetInputArrayToProcess(
                0, 0, 0,
                vtkDataObject::FIELD_ASSOCIATION_POINTS,
                "U");

            st->Update();
            scene.addData<vtkPolyDataMapper>(st->GetOutput(), vec3(0.5,0.5,0.5));
        }
      ++ap;

        {
            // auto seeds = vtkSmartPointer<vtkPointSource>::New();
            // seeds->SetCenter(toArray(vec3(0.5*sp().l_, -0.5*sp().w_, sp().Ldown_+0.5*sp().h_)));
            // seeds->SetRadius(0.2*sp().Lref_);
            // seeds->SetDistributionToUniform();
            // seeds->SetNumberOfPoints(100);
            auto seeds = vtkSmartPointer<vtkLineSource>::New();
            seeds->SetPoint1(toArray(vec3(-0.1*sp().Lupstream_, -0.05*sp().w_, sp().Ldown_)));
            seeds->SetPoint2(toArray(vec3(-0.1*sp().Lupstream_, -0.05*sp().w_, sp().Ldown_+(1.+1./double(ns))*Hs)));
            seeds->SetResolution(ns+1);

            auto st = vtkSmartPointer<vtkStreamTracer>::New();
            st->SetInputData(im);
            st->SetSourceConnection(seeds->GetOutputPort());
            st->SetIntegrationDirectionToBoth();
            st->SetMaximumPropagation(10.*sp().Ldownstream_);
            st->SetInputArrayToProcess(
                0, 0, 0,
                vtkDataObject::FIELD_ASSOCIATION_POINTS,
                "U");

            st->Update();
            scene.addData<vtkPolyDataMapper>(st->GetOutput(), vec3(0.5,0.5,0.5));
        }
        ++ap;


        scene.addDiagonalViews(
            *results,
            evalCS, camDist,
            executionPath() / "streamLines.png",
            "Stream lines around object",
            VTKOffscreenScene::ParallelScale(maxObjSize) );
        ++ap;

        // {
        //     camera->SetViewUp( toArray(vec3(0,0,1)) );
        //     camera->SetPosition( toArray(viewctr+vec3(-10.0*sp().l_,0,0)) );

        //     auto img = executionPath() / "streamLines_front.png";
        //     //      scene.fitAll();
        //     scene.setParallelScale(std::pair<double,double>(2.*sp().w_, 2.*sp().h_));
        //     scene.exportImage(img);
        //     results->insert(img.filename().stem().string(),
        //                     std::unique_ptr<Image>(new Image
        //                                            (
        //                                                executionPath(), img.filename(),
        //                                                "Stream lines (front view)", ""
        //                                                )));
        // }
        // ++ap;

        // {
        //     camera->SetViewUp( toArray(vec3(0,0,1)) );
        //     camera->SetPosition( toArray(viewctr+vec3(10.0*sp().l_,0,0)) );

        //     auto img = executionPath() / "streamLines_rear.png";
        //     //      scene.fitAll();
        //     scene.setParallelScale(std::pair<double,double>(2.*sp().w_, 2.*sp().h_));
        //     scene.exportImage(img);
        //     results->insert(img.filename().stem().string(),
        //                     std::unique_ptr<Image>(new Image
        //                                            (
        //                                                executionPath(), img.filename(),
        //                                                "Stream lines (rear view)", ""
        //                                                )));
        // }
        // ++ap;

        // {
        //     camera->SetViewUp( toArray(vec3(0,0,1)) );
        //     camera->SetPosition( toArray(viewctr+vec3(0,-10.0*sp().w_,0)) );

        //     auto name = "streamLines_side";

        //     scene.setParallelScale(std::pair<double,double>(2.*sp().l_, 2.*sp().h_));

        //     results->insert<Image>(name,
        //                            FileContainer(*scene.exportImage(), name),
        //                            "Stream lines (side view)", "" );
        // }
        // ++ap;

        // {
        //     camera->SetViewUp( toArray(vec3(0,1,0)) );
        //     camera->SetPosition( toArray(viewctr+vec3(0,0,10.0*sp().h_)) );

        //     auto img = executionPath() / "streamLines_top.png";
        //     //      scene.fitAll();
        //     scene.setParallelScale(std::pair<double,double>(2.*sp().l_, 2.*sp().w_));
        //     scene.exportImage(img);
        //     results->insert(img.filename().stem().string(),
        //                     std::unique_ptr<Image>(new Image
        //                                            (
        //                                                executionPath(), img.filename(),
        //                                                "Stream lines (top view)", ""
        //                                                )));
        // }
        // ++ap;

        // {
        //     camera->SetViewUp( toArray(vec3(0,0,1)) );
        //     camera->SetPosition( toArray(viewctr+10.*vec3(-sp().l_,-sp().w_,sp().h_)) );

        //     auto img = executionPath() / "streamLines_diag.png";
        //     //      scene.fitAll();
        //     double f=sqrt(2.);
        //     scene.setParallelScale(std::pair<double,double>(
        //         2.*std::max(f*sp().l_, f*sp().w_),
        //         2.*std::max(f*sp().l_, f*sp().h_)
        //         ));
        //     scene.exportImage(img);
        //     results->insert(img.filename().stem().string(),
        //                     std::unique_ptr<Image>(new Image
        //                                            (
        //                                                executionPath(), img.filename(),
        //                                                "Stream lines (isometric view)", ""
        //                                                )));
        // }
        // ++ap;

    }


    return results;
}


}
