
#include "base/exception.h"
#include "base/vtkrendering.h"

#include "vtkDataSetMapper.h"

using namespace insight;

int main(int argc, char*argv[])
{
    try
    {

        insight::assertion(
                    argc==2,
                    "specify source dir with Openfoam case as argument!");

        auto caseDir = boost::filesystem::path(argv[1])/
                "tutorial_chtMultiRegionSimpleFoam_heatExchanger";

        insight::assertion(
                    boost::filesystem::is_directory(caseDir),
                    "sample case directory "+caseDir.string()+" not found!" );

        OpenFOAMCaseScene scene( caseDir/"system"/"controlDict" );
        auto ofds = scene.ofcase()->GetOutput();
        scene.clearScene();

        auto idxs = MultiBlockDataSetExtractor(ofds).flatIndices(
            {"air", "internalMesh"} );
        std::cout<<"idx=";
        for (const auto& i: idxs)
        {
            std::cout<<" "<<i;
        }
        std::cout<<std::endl;

        auto exb = scene.extractBlock(idxs);
        exb->Update();
        exb->PrintSelf(std::cout, vtkIndent());

        FieldSelection ws("T", FieldSupport::Point, -1);
        auto ws_range = calcRange(ws, {}, {exb});

        FieldColor ws_color(ws, createColorMap(), ws_range);
        scene.addAlgo<vtkDataSetMapper>(exb, ws_color);
        scene.addColorBar(ws.fieldName(), ws_color.lookupTable(), 0.1, 0.1, true);

        auto camera = scene.activeCamera();
        camera->ParallelProjectionOn();
        camera->SetFocalPoint( toArray(vec3(0, 0, 0)) );
        camera->SetViewUp( toArray(vec3(0, 1, 0)) );
        camera->SetPosition( toArray(vec3(0, 0, 10)) );
        scene.fitAll();

    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
