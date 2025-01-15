#include "blockmeshvisualization.h"

#include "BRepBuilderAPI_MakeSolid.hxx"
#include "GeomAPI_Interpolate.hxx"
#include "BRepBuilderAPI_MakeEdge.hxx"
#include "BRepBuilderAPI_MakeVertex.hxx"
#include "BRepBuilderAPI_MakeWire.hxx"
#include "BRepCheck_Shell.hxx"
#include "GC_MakeArcOfCircle.hxx"

#include "base/exception.h"
#include "occinclude.h"
#include "cadfeatures/importsolidmodel.h"

namespace std
{

bool operator<( const insight::bmd::PointPair& p1, const insight::bmd::PointPair& p2)
{
    return (p1.first<p2.first) && (p1.second<p2.second);
}

}


namespace insight
{


namespace bmd
{


TopoDS_Edge blockMeshVisualization::addEdge(const arma::mat& p0, const arma::mat& p1)
{
    bmd::PointPair pp(p0, p1), pp2(p1, p0);
//    if (includedEdges.find(pp)==includedEdges.end()
//            &&
//        includedEdges.find(pp2)==includedEdges.end())
    {
        includedEdges.insert(pp);
        TopoDS_Edge edg;
        if (const auto* e = bm_.edgeBetween(p0, p1))
        {
            if (auto* se = dynamic_cast<const bmd::SplineEdge*>(e))
            {
                auto pts = se->allPoints();
                Handle_TColgp_HArray1OfPnt pts_col = new TColgp_HArray1OfPnt( 1, pts.size() );
                for ( int j=0; j<pts.size(); j++ )
                {
                    pts_col->SetValue ( j+1, to_Pnt ( pts[j] ) );
                }
                GeomAPI_Interpolate splbuilder ( pts_col, false, 1e-6 );
                splbuilder.Perform();
                Handle_Geom_BSplineCurve crv=splbuilder.Curve();
                edg=BRepBuilderAPI_MakeEdge ( crv, crv->FirstParameter(), crv->LastParameter() ).Edge();

            }
            else if (auto* ae = dynamic_cast<const bmd::ArcEdge*>(e))
            {
                auto crv = GC_MakeArcOfCircle(
                    to_Pnt(p0), to_Pnt(ae->midpoint()), to_Pnt(p1) ).Value();
                edg=BRepBuilderAPI_MakeEdge(crv).Edge();
            }
        }
        if (edg.IsNull())
        {
            if (arma::norm(p0-p1,2)>insight::SMALL)
                edg=BRepBuilderAPI_MakeEdge(to_Pnt(p0), to_Pnt(p1)).Edge();
        }

        return edg;
    }
}

blockMeshVisualization::blockMeshVisualization(const bmd::blockMesh& bm)
    : bm_(bm)
{
    for (const auto& bl: bm.allBlocks())
    {

        BRepBuilderAPI_Sewing sew(Precision::Confusion());

        for (const auto& flabel: {
             "a", "b", "c", "d", "e", "f"
             })
        {
            auto facepts = bl.face(flabel);

            for (const auto& v: facepts)
            {
                if (includedVertices.find(v)==includedVertices.end())
                {
                    includedVertices.insert(v);
                    // push_back ( BRepBuilderAPI_MakeVertex(to_Pnt(v)).Vertex() );
                }
            }

            TopTools_ListOfShape edgs;
            edgs.Append(addEdge(facepts[0], facepts[1]));
            edgs.Append(addEdge(facepts[1], facepts[2]));
            edgs.Append(addEdge(facepts[2], facepts[3]));
            edgs.Append(addEdge(facepts[3], facepts[0]));

            BRepBuilderAPI_MakeWire wb;
            TopoDS_Wire w;
            try
            {
                wb.Add(edgs);
                w=wb.Wire();
            }
            catch (...)
            {
                auto i=edgs.begin();
                throw insight::CADException(
                    {
                     {"e1", cad::Import::create(*(i)) },
                     {"e2", cad::Import::create(*(++i)) },
                     {"e3", cad::Import::create(*(++i)) },
                     {"e4", cad::Import::create(*(++i)) }
                    },
                    "failed to create wire"
                );
            }

            try
            {
                auto face = BRepBuilderAPI_MakeFace(w).Face();
                sew.Add( face );
            }
            catch(...)
            {
                throw insight::CADException(
                    {
                        {"w", cad::Import::create(w) }
                    },
                    "failed to create face from wire");
            }
        }

        sew.Perform();

        push_back(sew.SewedShape());
    }

//    for (const auto& e: bm.allEdges())
//    {
//        addEdge(e.c0(), e.c1());
//    }
}

}


} // namespace insight
