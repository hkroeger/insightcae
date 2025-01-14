#include "base/exception.h"
#include "base/boost_include.h"

#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"

using namespace insight;

int main(int argc, char*argv[])
{
    try
    {
        insight::assertion(
            argc==2,
            "specify source dir with Openfoam case as argument!");

        auto caseDir = boost::filesystem::path(argv[1])/
                       "tutorial_mixerVesselAMI2D";

        insight::assertion(
            boost::filesystem::is_directory(caseDir),
            "sample case directory "+caseDir.string()+" not found!" );

        ParallelTimeDirectories ptd(
            OpenFOAMCase(OFEs::getCurrentOrPreferred()),
            caseDir);

        insight::assertion(
            ptd.latestTimeNeedsReconst(),
            "expected latest time to be condidate!" );

        auto cands = ptd.newParallelTimes();
        for (const auto& c: cands)
        {
            std::cout << "cand: "<<c<<std::endl;
        }

        // git might not reproduce the time stamp upon clone...
//        insight::assertion(
//            cands.find("0")!=cands.end(),
//            "expected 0 to be candidate because newer files in proc0");

        insight::assertion(
            cands.find("1")!=cands.end(),
            "expected 0 to be candidate because missing polyMesh in ser dir");
        insight::assertion(
            cands.find("3")==cands.end(),
            "expected 3 to be NO candidate because up-to-date serial dir");
        insight::assertion(
            cands.find("5")!=cands.end(),
            "expected 5 to be candidate because missing serial dir");

    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
