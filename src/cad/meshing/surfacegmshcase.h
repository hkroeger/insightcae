#ifndef INSIGHT_CAD_SURFACEGMSHCASE_H
#define INSIGHT_CAD_SURFACEGMSHCASE_H

#include "meshing/gmshcase.h"


namespace insight {
namespace cad {


class SurfaceGmshCase
    : public GmshCase
{
    void setOpts(bool recombineTris);

public:
    SurfaceGmshCase(
            cad::ConstFeaturePtr part,
            const boost::filesystem::path& outputMeshFile,
            double Lmax, double Lmin,
            const std::string& name,
            bool keepDir=false,
            bool recombineTris = true
            );

    /**
     * @brief SurfaceGmshCase
     * @param part
     * @param surfaces
     * array of tuples (face set,L,name)
     * @param outputMeshFile
     * @param keepDir
     * @param recombineTris
     */
    SurfaceGmshCase(
            cad::ConstFeaturePtr part,
            const boost::filesystem::path& outputMeshFile,
            double Lmax, double Lmin,
            std::vector<std::tuple<FeatureSetPtr,std::string,double> > surfaces,
            bool keepDir=false,
            bool recombineTris = true
            );
};


} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_SURFACEGMSHCASE_H
