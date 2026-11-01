#ifndef INSIGHT_BMD_GEOMETRY_H
#define INSIGHT_BMD_GEOMETRY_H

#include <boost/filesystem.hpp>
#include <memory>
#include <mutex>
#include <string>

#include "vtkSmartPointer.h"
#include "vtkPolyData.h"

#include "openfoam/blockmesh/blockmeshprediction.h"

class vtkCellLocator;

namespace insight {
namespace bmd {


/**
 * @brief The Geometry class
 * a geometry surface for projections in blockMesh (triSurfaceMesh).
 * The label is the string value.
 * By default, the file has to be supplied externally.
 * Derived classes may supply the surface in memory (surface()):
 * then the projection is known before anything is written to disk
 * and the file can be written from it (writeFile).
 */
class Geometry
        : public std::string
{
    boost::filesystem::path fileName_;

    // cell locator on the surface, built on first use (shared by copies)
    struct Locator
    {
        std::mutex mutex;
        bool initialized = false;
        vtkSmartPointer<vtkCellLocator> locator;
    };
    std::shared_ptr<Locator> locator_;

public:
    Geometry(const std::string& label, const boost::filesystem::path& fileName);
    virtual ~Geometry();

    const boost::filesystem::path& fileName() const;

    /**
     * the triangulated surface (as written to the file),
     * null, if the file is supplied externally
     */
    virtual vtkSmartPointer<vtkPolyData> surface() const;

    /**
     * nearest point on the surface (empty, if no surface is available)
     */
    SurfaceProjection projection() const;

    /**
     * write the surface into directory dir under fileName().
     * Does nothing, if no surface is available.
     */
    void writeFile(const boost::filesystem::path& dir) const;
};


typedef std::shared_ptr<const Geometry> GeometryPtr;


} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_GEOMETRY_H
