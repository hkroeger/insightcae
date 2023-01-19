#include "surfacegmshcase.h"

#include "cadfeature.h"

namespace insight {
namespace cad {




void SurfaceGmshCase::setOpts(bool recombineTris)
{
    if (recombineTris)
    {
        insertLinesBefore(endOfMeshingOptions_, {
                              "Mesh.RecombinationAlgorithm = 0",
                              "Mesh.RecombineAll = 1",
                          });
    }
    insertLinesBefore(endOfMeshingOptions_, {
                          "Mesh.SecondOrderIncomplete=1",
                          "Mesh.Optimize = 1",
                          "Mesh.OptimizeNetgen = 1"
                      });
}




SurfaceGmshCase::SurfaceGmshCase(
        cad::ConstFeaturePtr part,
        const boost::filesystem::path& outputMeshFile,
        double Lmax, double Lmin,
        const std::string& name,
        bool keepDir,
        bool recombineTris
        )
    : cad::GmshCase(part, outputMeshFile,
                    Lmax, Lmin, keepDir)
{
    auto allFaces=part->allFaces();
    nameFaces(name, *allFaces);

    setOpts(recombineTris);
}




SurfaceGmshCase::SurfaceGmshCase(
        cad::ConstFeaturePtr part,
        const boost::filesystem::path &outputMeshFile,
        double Lmax, double Lmin,
        std::vector<std::tuple<cad::FeatureSetPtr, std::string, double> > surfaces,
        bool keepDir,
        bool recombineTris)
    : cad::GmshCase(part, outputMeshFile,
                    Lmax, Lmin, keepDir)
{
    for (const auto& surf: surfaces)
    {
        auto set = std::get<0>(surf);
        auto name = std::get<1>(surf);
        auto L = std::get<2>(surf);

        nameFaces(name, *set);
        if (L>=0) setFaceEdgeLen(name, L);
    }
    setOpts(recombineTris);
}




} // namespace cad
} // namespace insight
