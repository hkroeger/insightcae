
#include "base/tools.h"
#include "internalpressureloss.h"
#include "openfoam/snappyhexmesh.h"
#include "base/vtktransformation.h"


namespace insight
{


InternalPressureLoss::supplementedInputData::supplementedInputData(
    ParameterSetInput ip,
    const boost::filesystem::path &executionPath,
    ProgressDisplayer &prg )
    : supplementedInputDataDerived<Parameters>( ip.forward<Parameters>(), executionPath, prg )
{
    stldir_=snappyHexMeshFeats::geometryDir(OFEs::get(p().run.OFEname), executionPath);
    fn_inlet_="inlet";
    fn_outlet_="outlet";

    // Analyze geometry
    // Find:
    // * Domain BB
    // * Inlet hydraulic diam.


    prg.message("Analyzing geometry...");

    for (auto &w: p().geometry)
    {
        auto file = w.second.file;
        if (file->isValid())
        {
            auto tbb=file->geometry()->modelBndBox();
            std::cout<<w.first<<": BB="<<tbb<<std::endl;
            bb_.extend(tbb);
        }
    }



    L_=bb_.col(1)-bb_.col(0);

    if (L_(0)<1e-12)
        throw insight::Exception("model size in x direction is zero!");
    if (L_(1)<1e-12)
        throw insight::Exception("model size in y direction is zero!");
    if (L_(2)<1e-12)
        throw insight::Exception("model size in z direction is zero!");

    nx_=std::max(1, int(ceil(L_(0)/p().mesh.size)));
    ny_=std::max(1, int(ceil(L_(1)/p().mesh.size)));
    nz_=std::max(1, int(ceil(L_(2)/p().mesh.size)));

    pAmbient_=0.;

    if (const auto* thermsolve =
        boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
            &p().operation.thermalTreatment))
    {
        globalTmin=globalTmax=double(thermsolve->initialInternalTemperature)*si::degK;
        auto updateTmima = [&](si::Temperature T)
        {
            globalTmin=std::min(globalTmin, T);
            globalTmin=std::min(globalTmin, T);
        };

        // updateTmima(double(thermsolve->inletTemperature)*si::degK);

        for (auto& wbc: thermsolve->BCs)
        {
            if (const auto* wallfixedT =
                boost::get<Parameters::operation_type::thermalTreatment_solve_type::BCs_default_fixedTemperature_type>(
                    &wbc.second))
            {
                updateTmima(double(wallfixedT->temperature)*si::degK);
            }
        }

        globalTmin *= 0.5;
        globalTmax *= 1.5;


        reportSupplementQuantity("globalTmin", toValue(globalTmin, si::degK), "lower temperature clip value", "K");
        reportSupplementQuantity("globalTmax", toValue(globalTmax, si::degK), "upper temperature clip value", "K");

        if (const auto *buoy =
            boost::get<Parameters::operation_type::thermalTreatment_solve_type::includeBuoyancy_yes_type>(
                &thermsolve->includeBuoyancy))
        {
            pAmbient_=buoy->outletPressure;
        }
    }
}


}
