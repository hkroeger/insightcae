#ifndef INSIGHT_BMD_BLOCKMESHCADGEOMETRY_H
#define INSIGHT_BMD_BLOCKMESHCADGEOMETRY_H

#include "toolkit_cad_export.h"

#include "openfoam/blockmesh/geometry.h"
#include "cadtypes.h"

#include <mutex>

namespace insight {
namespace bmd {


/**
 * @brief The CADGeometry class
 * a projection geometry for blockMesh, which is given by a CAD feature.
 * The feature is triangulated (on first use) with the same tolerance,
 * which is used for writing the STL file. Thus the projection
 * (mesh prediction, visualization) is available before anything is written to disk
 * and matches the surface, onto which blockMesh projects.
 */
class TOOLKIT_CAD_EXPORT CADGeometry
        : public Geometry
{
    cad::FeaturePtr feature_;
    double tolerance_;

    struct Surface
    {
        std::mutex mutex;
        vtkSmartPointer<vtkPolyData> surface;
    };
    std::shared_ptr<Surface> surface_;

public:
    /**
     * @param tolerance
     * absolute deflection of the triangulation
     */
    CADGeometry(
        const std::string& label,
        const boost::filesystem::path& fileName,
        cad::FeaturePtr feature,
        double tolerance );

    inline const cad::FeaturePtr& feature() const { return feature_; }
    inline double tolerance() const { return tolerance_; }

    vtkSmartPointer<vtkPolyData> surface() const override;
};


typedef std::shared_ptr<CADGeometry> CADGeometryPtr;


} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_BLOCKMESHCADGEOMETRY_H
