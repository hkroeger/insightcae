#include "framemesh.h"

#include "meshing.h"

#include "BRepExtrema_DistShapeShape.hxx"
#include "Poly_Triangulation.hxx"
#include "Poly.hxx"

#include "code_aster/codeastermeshfile.h"
#include "cadfeatures/booleanintersection.h"
#include "cadfeatures/transform.h"

#include "datum.h"
#include "geotest.h"

#include "polytriangulationnodeiterator.h"
#include "polytriangulationelementiterator.h"

//#if defined(HAVE_MED)
//#undef HAVE_MED
//#endif

#if defined(HAVE_MED)

extern "C" {
#include "med.h"
#include "medfile.h"
}


struct MeshInfo
{
    std::string meshGroupName;
    insight::OCC::PolyTriangulationNodeIterator nodeIt;
    insight::OCC::PolyTriangulationElementIterator triIt;
};


void writeMED(
        const boost::filesystem::path& fileName,
        const std::string& meshName,
        const std::vector<MeshInfo>& meshes
        )
{

    med_int major = MED_MAJOR_NUM, minor = MED_MINOR_NUM, release = MED_RELEASE_NUM;

    med_idt medfile = MEDfileVersionOpen(
                (char *)fileName.c_str(), MED_ACC_CREAT,
                major, minor, release );

    if(medfile < 0)
    {
        throw insight::Exception(str(boost::format("Unable to open file '%s'") % fileName));
    }

    // write header
    if (MEDfileCommentWr(
                medfile,
                "MED file generated by InsightCAE") < 0)
    {
        throw insight::Exception("Unable to write MED descriptor");
    }

    // Gmsh always writes 3D unstructured meshes
    char dtUnit[MED_SNAME_SIZE + 1] = "";
    char axisName[3 * MED_SNAME_SIZE + 1] = "";
    char axisUnit[3 * MED_SNAME_SIZE + 1] = "";
    if(MEDmeshCr(
                medfile,
                meshName.c_str(),
                3, 3,
                MED_UNSTRUCTURED_MESH,
                "Mesh created by InsightCAD",
                dtUnit,
                MED_SORT_DTIT, MED_CARTESIAN,
                axisName, axisUnit) < 0)
    {
        throw insight::Exception("Could not create MED mesh");
    }

    // always create a "0" family, with no groups or attributes
    if(MEDfamilyCr(medfile, meshName.c_str(), "F_0", 0, 0, "") < 0)
    {
        throw insight::Exception("Could not create MED family 0");
    }

    std::vector<int> nofs={0};
    std::vector<med_float> coord;

    for (const auto& m: meshes)
    {
        int i = &m - &meshes.front();

        std::cout<<"create family "<<m.meshGroupName<<std::endl;
        // write the families
        if ( MEDfamilyCr(
                 medfile, meshName.c_str(),
                 ("F_"+m.meshGroupName).c_str(), -(i+1),
                 1, m.meshGroupName.c_str()
                 ) < 0)
        {
            throw insight::Exception("Could not create family "+m.meshGroupName);
        }

        {
            int n=0;
            for (auto pi=m.nodeIt; pi!=insight::OCC::PolyTriangulationNodeIterator(); ++pi)
            {
                std::cout<<"x/y/z="<<pi->X()<<"/"<<pi->Y()<<"/"<<pi->Z()<<std::endl;
                coord.push_back( pi->X() );
                coord.push_back( pi->Y() );
                coord.push_back( pi->Z() );
                ++n;
            }
            nofs.push_back(n+nofs.back());

            if(coord.empty())
            {
                throw insight::Exception("No nodes to write in MED mesh");
            }
        }
    }

    {
        std::vector<med_int> fam(nofs.back(), 0);

        std::cout<<"write nodes n="<<fam.size()<<"/"<<coord.size()<<std::endl;
        // write the nodes
        if ( MEDmeshNodeWr(
                 medfile,
                 meshName.c_str(),
                 MED_NO_DT,
                 MED_NO_IT,
                 0.,
                 MED_FULL_INTERLACE,
                 (med_int)fam.size(), &coord[0],
                 MED_FALSE, "",
                 MED_FALSE, 0,
                 MED_TRUE, &fam[0]
                 ) < 0)
        {
            throw insight::Exception("Could not write nodes");
        }
    }

    med_geometry_type typ = MED_TRIA3;

    std::vector<med_int> conn, fam;
    for (const auto& m: meshes)
    {
        int i = &m - &meshes.front();

        // triangles

        for (auto j=m.triIt; j!=insight::OCC::PolyTriangulationElementIterator(); ++j)
        {
            std::cout<<"i/j/k="<<nofs[i] +j->Value(1)<<"/"<<nofs[i] +j->Value(2)<<"/"<<nofs[i] +j->Value(3)<<std::endl;
            conn.push_back( nofs[i] + j->Value(1) );
            conn.push_back( nofs[i] + j->Value(2) );
            conn.push_back( nofs[i] + j->Value(3) );
            fam.push_back(-(i+1));
        }
    }


    if (MEDmeshElementWr(
                medfile,
                meshName.c_str(),
                MED_NO_DT, MED_NO_IT, 0.,
                MED_CELL, typ,
                MED_NODAL,
                MED_FULL_INTERLACE,
                (med_int)fam.size(), &conn[0],
                MED_FALSE, 0,
                MED_FALSE, 0,
                MED_TRUE, &fam[0]) < 0)
    {
        throw insight::Exception("Could not write MED elements");
    }


    if(MEDfileClose(medfile) < 0) {
        throw insight::Exception(
                    str(boost::format("Unable to close file '%s'") % fileName)
                    );
    }

}






namespace std
{

bool operator<(const TopoDS_Shape& s1, const TopoDS_Shape& s2)
{
    return s1.HashCode(INT_MAX) < s2.HashCode(INT_MAX);
}

}

bool operator<(const gp_Pnt& v1, const gp_Pnt& v2)
{
    if ( fabs(v1.X() - v2.X())<Precision::Confusion() )
      {
        if ( fabs(v1.Y() - v2.Y())<Precision::Confusion() )
          {
            if (fabs(v1.Z()-v2.Z())<Precision::Confusion())
            {
                //return v1.instance_ < v2.instance_;
                return false;
            }
            else return v1.Z()<v2.Z();
          }
        else return v1.Y()<v2.Y();
      }
    else return v1.X()<v2.X();
}


namespace insight {
namespace cad {




class FindIntersections
{
public:
    typedef std::pair<gp_Pnt, double> IntersectionLocation;
    typedef std::set<IntersectionLocation> IntersectionLocations;

private:
    IntersectionLocations intersectionsOnE1_, intersectionsOnE2_;
public:
    FindIntersections(const TopoDS_Edge& e1, const TopoDS_Edge& e2);

    const IntersectionLocations& intersectionsOnE1()
    { return intersectionsOnE1_; }

    const IntersectionLocations& intersectionsOnE2()
    { return intersectionsOnE2_; }

    static bool isOnEndpoint(const gp_Pnt& p, const TopoDS_Edge& e);

    static void append(const IntersectionLocations& newIts,
                       IntersectionLocations& its);
};





FindIntersections::FindIntersections(const TopoDS_Edge& e, const TopoDS_Edge& e2)
{
    BRepExtrema_DistShapeShape dss(e, e2);
    for (int s=1; s<=dss.NbSolution(); ++s)
    {
        auto p1=dss.PointOnShape1(s);
        auto p2=dss.PointOnShape2(s);
        if ( p1.Distance(p2) < Precision::Confusion() )
        {
            if ( dss.SupportTypeShape1(s)==BRepExtrema_IsOnEdge /*!isOnEndpoint(p1, e)*/)
            {
                double t;
                dss.ParOnEdgeS1(s, t);
                intersectionsOnE1_.insert(
                            std::make_pair(p1, t)
                            );
            }
            if ( dss.SupportTypeShape2(s)==BRepExtrema_IsOnEdge /*!isOnEndpoint(p1, e2)*/)
            {
                double t;
                dss.ParOnEdgeS2(s, t);
                intersectionsOnE2_.insert(
                            std::make_pair(p1, t)
                            );
            }
        }
    }
    std::cout<<"nb inter = "<<intersectionsOnE1_.size()<<"/"<<intersectionsOnE2_.size()<<std::endl;
}




bool FindIntersections::isOnEndpoint(const gp_Pnt& p, const TopoDS_Edge& e)
{
    auto fv = BRep_Tool::Pnt(TopExp::FirstVertex(e));
    auto lv = BRep_Tool::Pnt(TopExp::LastVertex(e));

    if (fv.SquareDistance(p)<Precision::Confusion()) return true;
    if (lv.SquareDistance(p)<Precision::Confusion()) return true;
    return false;
}




void FindIntersections::append(
        const IntersectionLocations& newIts,
        IntersectionLocations& its )
{
    std::copy(newIts.begin(), newIts.end(),
              std::inserter(its, its.begin()));
}




void splitEdge(
        const TopoDS_Edge& e,
        const FindIntersections::IntersectionLocations& spl,
        std::set<TopoDS_Edge>& result
        );


void splitEdge(
        const TopoDS_Edge& e,
        const FindIntersections::IntersectionLocations& spl,
        std::set<TopoDS_Edge>& result
        )
{
    if (spl.size())
    {
        std::set<double> splt;

        Handle_Geom_Curve crv;
        {
            double fp, lp;
            crv = BRep_Tool::Curve(e, fp, lp);
            splt.insert(fp);
            splt.insert(lp);
        }

        std::transform(spl.begin(), spl.end(),
                       std::inserter(splt, splt.begin()),
                       [](const FindIntersections::IntersectionLocations::value_type& spll)
                        {
                            return spll.second;
                        }
        );

        for ( auto fp=splt.begin(); fp!=(--splt.end()); ++fp)
        {
            auto np=fp; ++np;
            result.insert(
                        BRepBuilderAPI_MakeEdge(crv, *fp, *np)
                        .Edge() );
        }
    }
    else
    {
        result.insert(e);
    }
}


}
}

#endif





namespace insight {
namespace cad {


void applyLocation(Poly_Triangulation& pt, const TopLoc_Location& loc)
{
    for (int i=1; i<=pt.NbNodes(); ++i)
    {
        pt.ChangeNode(i)=pt.Node(i).Transformed(loc);
    }
}

defineType(FrameMesh);

size_t FrameMesh::calcHash() const
{
    return 0;
}

void FrameMesh::build()
{
#if defined(HAVE_MED)
    typedef std::set<TopoDS_Edge> EdgeSet;

    EdgeSet modelEdges;
    std::map<TopoDS_Edge, std::pair<cad::FeaturePtr,double> > crossSectionModels;
    // unify all into compound
    {
        for (auto& e: edges_)
        {
            TopoDS_Shape s = boost::fusion::get<0>(e)->shape();
            for ( TopExp_Explorer ex(s, TopAbs_EDGE);
                  ex.More(); ex.Next() )
            {
                auto edg = TopoDS::Edge(ex.Current());
                modelEdges.insert(edg);
                if (boost::fusion::get<1>(e))
                {
                    crossSectionModels[edg]=std::make_pair(
                            boost::fusion::get<0>(*boost::fusion::get<1>(e)),
                            boost::fusion::get<1>(*boost::fusion::get<1>(e))->value()
                            );
                }
            }
        }
    }

    // break all edges at crossing points
    enum VertexType { Hinge, Stiff };

    std::map<TopoDS_Edge, FindIntersections::IntersectionLocations>
            intersections;
    std::map<TopoDS_Vertex, VertexType> vertexTypes;

//    for ( TopExp_Explorer ex(model, TopAbs_VERTEX);
//          ex.More(); ex.Next() )
//    {
//        auto v = TopoDS::Vertex(ex.Current());
//        vertexTypes[v]=Stiff;
//    }


    for ( auto& e: modelEdges )
    {
        // check, if other edges cross this edge
        //  >> if so, add break points to list
        for ( auto& e2: modelEdges )
        {
            if ( e != e2 )
            {
                FindIntersections its(e, e2);
                FindIntersections::append(
                            its.intersectionsOnE1(),
                            intersections[e]);
                FindIntersections::append(
                            its.intersectionsOnE2(),
                            intersections[e2]);
            }
        }
    }

    // break edges
    EdgeSet meshEdges;
    for (auto& e: modelEdges)
    {
        splitEdge(e, intersections[e], meshEdges);
    }

//    {
//        BRep_Builder bb;
//        TopoDS_Compound result;
//        bb.MakeCompound ( result );
//        for (const auto& e: meshEdges)
//        {
//            bb.Add ( result, e );
//        }
//        BRepTools::Write(result, "broken.brep");
//    }
//    {
//        BRep_Builder bb;
//        TopoDS_Compound result;
//        bb.MakeCompound ( result );
//        bb.Add ( result, model_->shape() );
//        BRepTools::Write(result, "unbroken.brep");
//    }

    // do meshing
    CodeAsterMeshFile mesh;
    for (const auto& e: meshEdges)
    {
        BRepAdaptor_Curve adapt(e);
        GCPnts_UniformAbscissa abs(
                    adapt, L_->value()
                    );

        std::vector<arma::mat> pts;
        for (int i=1; i<=abs.NbPoints(); i++)
        {
            pts.push_back( insight::Vector(adapt.Value(abs.Parameter(i))) );
        }
        PolylineMesh m(mesh, pts, false, false);
    }

    // determine cross section properties
    std::vector<MeshInfo> xsecmeshes;
    int k=0;
    for (const auto& e: meshEdges)
    {
        auto i=crossSectionModels.find(e);
        if (i!=crossSectionModels.end())
        {
            cad::FeaturePtr xsecf = i->second.first;
            double x = i->second.second;
            insight::assertion(
                        ( (x>=0.0) && (x<=1.0) ),
                        "the coordinate along the edge has to be between 0 and 1!" );

            double t0, t1;
            auto crv = BRep_Tool::Curve(e, t0, t1);
            double t = x*t1 + (1.-x)*t0;

            gp_Pnt p;
            gp_Vec tan;
            crv->D1(t, p, tan);

            auto xsec = cad::BooleanIntersection::create_plane(
                        xsecf,
                        std::make_shared<cad::DatumPlane>(
                            cad::matconst(insight::Vector(p)),
                            cad::matconst(insight::Vector(tan.Normalized()))
                            )
                        );

            gp_Trsf trsf;
            trsf.SetTransformation(
                        gp_Ax3(p, tan),
                        gp_Ax3(gp::Origin(), gp::DZ())
                        );
            xsec=cad::Transform::create_trsf( xsec, trsf.Inverted() );


            Poly_ListOfTriangulation triangulations;
            TopoDS_Shape s = xsec->shape();
            for (TopExp_Explorer ex(s, TopAbs_FACE); ex.More(); ex.Next())
            {
                auto f=TopoDS::Face(ex.Current());
                auto bnd = getBoundingBox(f, 1e-3); // enforce triangulation

                TopLoc_Location loc;
                auto mesh = BRep_Tool::Triangulation(f,loc);
                applyLocation(*mesh, loc);

                insight::assertion(
                            !mesh.IsNull(),
                            "shape has no triangulation!" );

                std::cout<<"add mesh with nodes="<<mesh->NbNodes()<<" / faces="<<mesh->NbTriangles()<<std::endl;
                triangulations.Append(mesh);
            }

            insight::assertion(
                        triangulations.Extent()>=1,
                        "there are no triangulations!" );

            auto entireMesh = Poly::Catenate(triangulations);
            std::cout<<"full mesh with nodes="<<entireMesh->NbNodes()<<" / faces="<<entireMesh->NbTriangles()<<std::endl;

            xsecmeshes.push_back(
                        {
                            str(boost::format("xsec_%d")%k),
                            OCC::PolyTriangulationNodeIterator(entireMesh),
                            OCC::PolyTriangulationElementIterator(entireMesh)
                        });

            ++k;
        }
    }


    writeMED("test.med", "xsecs", xsecmeshes);

    mesh.write(".", 20);
#else
    throw insight::Exception("InsightCAE has to be compiled with MED support to use the Frame Mesh export feature.");
#endif
}



FrameMesh::FrameMesh
(
  const boost::filesystem::path& outpath,
  ScalarPtr L,
  EdgesDesc edges
//  FeaturePtr model,
//  const FrameMeshGroupDefinitions& v_e_groups
)
    : outpath_(outpath),
      L_(L),
      edges_(edges)
//      model_(model),
//      v_e_groups_(v_e_groups)
{}

void FrameMesh::write(std::ostream& ) const
{}

} // namespace cad
} // namespace insight
