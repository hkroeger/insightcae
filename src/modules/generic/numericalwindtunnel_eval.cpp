
#include "base/cppextensions.h"
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
#include "openfoam/openfoamboundarydict.h"

namespace insight
{



ResultSetPtr NumericalWindtunnel::evaluateResults(OpenFOAMCase& cm, ProgressDisplayer& pp)
{
    auto results=insight::OpenFOAMAnalysis::evaluateResults(cm, pp);

    auto ap = pp.forkNewAction(7, "Evaluation");

    // get full name of car patch (depends on STL file)
    OpenFOAMBoundaryDict boundaryDict(cm,executionPath());

    std::set<std::string> objectPatchNames;
    for (auto& g: sp().geometry_)
    {
        auto pns=boundaryDict.expandPatchNamePattern(g.first+".*");
        objectPatchNames.insert(pns.begin(), pns.end());
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
        projDir,
        std::container_type_cast<std::vector<std::string> >(
            objectPatchNames) );

    double A=Ah(Ah.n_rows-1,1);
    ++ap;


    double Re=p().operation.v*sp().Lref_/p().fluid.nu;
    results->insert<ScalarResult>("Re", Re, "Reynolds number", "", "");
    results->insert<ScalarResult>("Afront", A, "Projected frontal area", "", "$m^2$");

    ap.message("Evaluating forces");

    auto evalForces = [&](const std::string& FOname, const std::string& description)
    {
        auto sec=std::make_unique<ResultSection>("Forces on "+description);

        arma::mat f=forces::readForces(cm, executionPath(), FOname);
        arma::mat t = f.col(0);
        ++ap;

        double mult = p().mesh.longitudinalSymmetry ? 2.0 : 1.0;

        arma::mat Rtot = (f.col(1)+f.col(4)) *mult;
        arma::mat Flat = (f.col(2)+f.col(5)) *mult;
        arma::mat Mz = (p().mesh.longitudinalSymmetry ? 0. : 1.) *
                       ( f.col(9)+f.col(12) );
        arma::mat L = (f.col(3)+f.col(6)) *mult;

        sec->insert<ScalarResult>("Rtot", Rtot(Rtot.n_rows-1),
                                  "Total resistance on "+description,
                                  "The resistance force is along the flow direction, i.e. along the X axis, regardless of the attitude (yaw angle).",
                                  "N");
        sec->insert<ScalarResult>("Flat", Flat(Flat.n_rows-1),
                                  "Lateral force on "+description,
                                  "The lateral force is always measured orthogonal to the flow direction, i.e. along the Y axis, regardless of the attitude (yaw angle).",
                                  "N");
        sec->insert<ScalarResult>("Myaw", Mz(Mz.n_rows-1),
                                  "Yaw moment on "+description,
                                  "The yaw moment is measured around the Z axis,",
                                  "Nm");
        sec->insert<ScalarResult>("L", L(L.n_rows-1),
                                  "Lifting force on "+description,
                                  "The lifting force is measured along the Z axis, positive upward.", "N");

        double denom=(0.5*p().fluid.rho*pow(p().operation.v,2)*A);

        if (fabs(denom)<SMALL)
        {
            insight::Warning(
                "denominator for force normalization is zero!"
                " Skipping calculation of force coefficients" );
        }
        else
        {
            double cr=Rtot(Rtot.n_rows-1) / denom;
            sec->insert<ScalarResult>(
                "cr", cr,
                "Resistance coefficient of "+description, "", "");

            double cl=L(L.n_rows-1) / denom;
            sec->insert<ScalarResult>(
                "cl", cl, "Lifting coefficient of "+description,
                "with respect to projected area of entire assembly and forward velocity", "");

            double cs=Flat(Flat.n_rows-1) / denom;
            sec->insert<ScalarResult>(
                "cs", cs, "Lateral forces coefficient of "+description,
                "with respect to projected area of entire assembly and forward velocity", "");

        }

        double Pe=Rtot(Rtot.n_rows-1) * p().operation.v;
        sec->insert<ScalarResult>(
            "Pe", Pe,
            "Effective power $P_e=R_{tot} v$ of "+description,
            "", "W");

        ap.message("Creating resistance plot");
        // Resistance convergence
        addPlot
            (
                *sec, executionPath(), "chartResistance",
                "Iteration", "F [N]",
                {
                    PlotCurve( arma::mat(join_rows(t, Rtot)), "Rtot", "w l lw 2 t 'Total resistance'"),
                    PlotCurve( arma::mat(join_rows(t, Flat)), "Flat", "w l lw 2 t 'Lateral force'"),
                    PlotCurve( arma::mat(join_rows(t, L)),    "L", "w l lw 2 t 'Lift force'"),
                    PlotCurve( arma::mat(join_rows(t, Mz)),   "Myaw", "w l axes x1y2 lw 2 t 'Yaw moment'")
                },
                "Convergence history of forces on "+description
                );

        results->insert(FOname, std::move(sec));
    };

    evalForces(sp().FOname_allObjects, "entire assembly");
    if (sp().geometry_.size()>1)
    {
        for (auto& g: sp().geometry_)
        {
            evalForces("forces_"+g.first, g.first);
        }
    }

    ++ap;


    ap.message("Rendering images");
    {
        // A renderer and render window
        OpenFOAMCaseScene scene( executionPath()/"system"/"controlDict" );

        auto patches = scene.patches("("+boost::join(objectPatchNames, "|")+")");

        FieldSelection sl_field("p", FieldSupport::OnPoint, -1);
        auto sl_range=calcRange(sl_field, {patches}, {});
        auto sl_cm=createColorMap();
        FieldColor sl_fc(sl_field, sl_cm, sl_range);

        scene.addData<vtkDataSetMapper>(patches, sl_fc);
        scene.addColorBar("Pressure\n[m^2/s^2]", sl_cm);

        auto camera = scene.activeCamera();
        camera->ParallelProjectionOn();

        insight::CoordinateSystem evalCS(
            vec3(0.5*sp().l_, 0, sp().Ldown_+0.5*(sp().hup_+sp().dlo_)),
            vec3X(), vec3Z() );
        double maxObjSize=std::max(sp().l_, std::max(sp().w_, (sp().hup_+sp().dlo_)));
        double camDist=10.*maxObjSize;


        auto im = scene.internalMesh();

        double Hs=1.25*(sp().hup_+sp().dlo_);
        int ns=50;
        {
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

    }

    if (p().eval.evaluateMeanResistance)
    {
        auto sec=std::make_unique<ResultSection>("Mean Resistance");

        double FD=results->getScalar(sp().FOname_allObjects+"/Rtot");
        sec->insert<ScalarResult>("FD", FD, "total drag", "", "N");

        double L=sp().Lupstream_+sp().l_+sp().Ldownstream_;
        double W=(sp().Laside_+0.5*sp().w_)*(p().mesh.longitudinalSymmetry?1.:2.);
        double A=L*W;
        sec->insert<ScalarResult>("A", A, "surface area", "", "m^2");

        double tauw=FD/A;
        sec->insert<ScalarResult>("tauw", tauw, "mean wall shear stress", "", "N/m^2");

        double utau=sqrt(tauw/p().fluid.rho);
        sec->insert<ScalarResult>("utau", utau, "wall shear stress velocity", "", "m/s");

        double Retau=utau*sp().Hdom_/p().fluid.nu;
        sec->insert<ScalarResult>("Retau", Retau, "Shear stress Reynolds Number", "", "");

        const double kappa=0.41;

        double Cplus=
            p().operation.v/utau
            -(1./kappa)*log(Retau)
            -( -1.7 );
        sec->insert<ScalarResult>("Cplus", Cplus, "", "", "");

        double ksplus=exp(-kappa*(Cplus-8.))-3.4;
        sec->insert<ScalarResult>("ksplus", ksplus, "roughness", "", "");

        double ks=ksplus*p().fluid.nu/utau;
        sec->insert<ScalarResult>("ks", ks, "corresponding roughness", "", "m");

        double z0=ks*0.5/9.793;
        sec->insert<ScalarResult>("z0", z0, "aerodynamic roughness", "", "m");

        results->insert("meanResistance", std::move(sec));
    }


    return results;
}


}
