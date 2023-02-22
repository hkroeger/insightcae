#ifndef INSIGHT_CAD_CADEMESH_H
#define INSIGHT_CAD_CADEMESH_H

#include "openfoam/openfoamtools.h"
#include "cadtypes.h"

namespace insight {
namespace cad {

class CADEMesh : public eMesh
{
    void add(const TopoDS_Shape& shape, double tol);
public:
    CADEMesh(const TopoDS_Shape& shape, double tol);
    CADEMesh(cad::FeaturePtr feat, double tol);
};


} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_CADEMESH_H
