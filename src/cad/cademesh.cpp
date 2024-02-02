#include "cademesh.h"

#include "cadfeature.h"

namespace insight {
namespace cad {


void CADEMesh::add(const TopoDS_Shape& shape, double tol)
{
    for (TopExp_Explorer ex(shape, TopAbs_EDGE); ex.More(); ex.Next())
    {
        TopoDS_Edge e = TopoDS::Edge(ex.Current());
        BRepAdaptor_Curve adapt(e);

        GCPnts_UniformAbscissa abs(
                    adapt, tol,
                    adapt.FirstParameter(),
                    adapt.LastParameter() );

        int iofs=nPoints();
        for (int i=0; i<abs.NbPoints(); ++i)
        {
            auto pi = adapt.Value(abs.Parameter(i+1));
            points_.push_back(vec3(pi));
            if (i>0) edges_.push_back({iofs+i-1, iofs+i});
        }
    }
}


CADEMesh::CADEMesh(const TopoDS_Shape& shape, double tol)
{
    add(shape, tol);
}

CADEMesh::CADEMesh(cad::FeaturePtr feat, double tol)
{
    add(feat->shape(), tol);
}


} // namespace cad
} // namespace insight
