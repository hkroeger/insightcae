#include "framemesh.h"

#include "meshing.h"

#include "BRepExtrema_DistShapeShape.hxx"
#include "Poly_Triangulation.hxx"
#include "Poly.hxx"

#include "code_aster/codeastermeshfile.h"

#include "cadfeatures/booleanintersection.h"
#include "datum.h"


extern "C" {
#include "med.h"
#include "medfile.h"
}

#if(MED_MAJOR_NUM == 3)
// To avoid too many ifdefs below we use defines for the bits of the
// API that did not change too much between MED2 and MED3. If we remove
// MED2 support at some point, please remove these defines and replace
// the symbols accordingly.
#define med_geometrie_element med_geometry_type
#define med_maillage med_mesh_type
#define MED_TAILLE_NOM MED_NAME_SIZE
#define MED_TAILLE_LNOM MED_LNAME_SIZE
#define MED_TAILLE_DESC MED_COMMENT_SIZE
#define MED_NON_STRUCTURE MED_UNSTRUCTURED_MESH
#define MED_LECTURE MED_ACC_RDONLY
#define MED_CREATION MED_ACC_CREAT
#define MEDouvrir MEDfileOpen
#define MEDversionDonner MEDlibraryNumVersion
#define MEDversionLire MEDfileNumVersionRd
#define MEDnMaa MEDnMesh
#define MEDfermer MEDfileClose
#define MEDnFam MEDnFamily
#define MEDfichDesEcr MEDfileCommentWr
#endif


template<class NodeIterator, class TriIterator>
void writeMED(
        const boost::filesystem::path& fileName,
        const std::string& meshGroupName,
        NodeIterator nodeIt,
        TriIterator triIt
        )
{
////    if(fam.empty()) return;

////   #if(MED_MAJOR_NUM == 3)
////     if(MEDmeshElementWr(fid, meshName, MED_NO_DT, MED_NO_IT, 0., MED_CELL, type,
////                         MED_NODAL, MED_FULL_INTERLACE, (med_int)fam.size(),
////                         &conn[0], MED_FALSE, 0, MED_FALSE, 0, MED_TRUE,
////                         &fam[0]) < 0)
////   #else
////     if(MEDelementsEcr(fid, meshName, (med_int)3, &conn[0], MED_FULL_INTERLACE, 0,
////                       MED_FAUX, 0, MED_FAUX, &fam[0], (med_int)fam.size(),
////                       MED_MAILLE, type, MED_NOD) < 0)
////   #endif
////       throw insight::Exception("Could not write MED elements");
////   }


//   #if(MED_MAJOR_NUM == 3) && (MED_MINOR_NUM >= 3)
     // MEDfileVersionOpen actually appeared in MED 3.2.1
     med_int major = MED_MAJOR_NUM, minor = MED_MINOR_NUM, release = MED_RELEASE_NUM;
//     if(CTX::instance()->mesh.medFileMinorVersion >= 0){
//       minor = (int)CTX::instance()->mesh.medFileMinorVersion;
//       Msg::Info("Forcing MED file version to %d.%d", major, minor);
//     }
     med_idt fid = MEDfileVersionOpen((char *)fileName.c_str(), MED_ACC_CREAT,
                                      major, minor, release);
//   #else
//     med_idt fid = MEDouvrir((char *)fileName.c_str(), MED_CREATION);
//   #endif
     if(fid < 0)
     {
       throw insight::Exception(str(boost::format("Unable to open file '%s'") % fileName));
     }

     // write header
     if (MEDfileCommentWr(
                 fid,
                 "MED file generated by Gmsh") < 0) {
       throw insight::Exception("Unable to write MED descriptor");
     }

     std::string strMeshName = meshGroupName;
     char *meshName = (char *)strMeshName.c_str();

     // Gmsh always writes 3D unstructured meshes
     char dtUnit[MED_SNAME_SIZE + 1] = "";
     char axisName[3 * MED_SNAME_SIZE + 1] = "";
     char axisUnit[3 * MED_SNAME_SIZE + 1] = "";
     if(MEDmeshCr(
                 fid,
                 meshName,
                 3, 3,
                 MED_UNSTRUCTURED_MESH,
                 "Mesh created by InsightCAD",
                 dtUnit,
                 MED_SORT_DTIT, MED_CARTESIAN,
                 axisName, axisUnit) < 0)
     {
       throw insight::Exception("Could not create MED mesh");
     }
//     // if there are no physicals we save all the elements
//      if(noPhysicalGroups()) saveAll = true;

//      // index the vertices we save in a continuous sequence (MED
//      // connectivity is given in terms of vertex indices)
//      indexMeshVertices(saveAll);

//      // get a vector containing all the geometrical entities in the
//      // model (the ordering of the entities must be the same as the one
//      // used during the indexing of the vertices)
//      std::vector<GEntity *> entities;
//      getEntities(entities);

//      std::map<GEntity *, int> families;
      // write the families
      {
      // always create a "0" family, with no groups or attributes

        if(MEDfamilyCr(fid, meshName, "F_0", 0, 0, "") < 0)
        {
            throw insight::Exception("Could not create MED family 0");
        }

//        // create one family per elementary entity, with one group per
//        // physical entity and no attributes
//        for(unsigned int i = 0; i < entities.size(); i++) {
//          if(saveAll || entities[i]->physicals.size()) {
//            int num = -((int)families.size() + 1);
//            families[entities[i]] = num;
//            std::ostringstream fs;
//            fs << entities[i]->dim() << "D_" << entities[i]->tag();
//            std::string familyName = "F_" + fs.str();
//            std::string groupName;
//            for(unsigned j = 0; j < entities[i]->physicals.size(); j++) {
//              std::string tmp =
//                getPhysicalName(entities[i]->dim(), entities[i]->physicals[j]);
//              if(tmp.empty()) { // create unique name
//                std::ostringstream gs;
//                gs << entities[i]->dim() << "D_" << entities[i]->physicals[j];
//                groupName += "G_" + gs.str();
//              }
//              else
//                groupName += tmp;
//              groupName.resize((j + 1) * MED_TAILLE_LNOM, ' ');
//            }
////    #if(MED_MAJOR_NUM == 3)
//            if(MEDfamilyCr(
//                        fid,
//                        meshName,
//                        familyName.c_str(), (med_int)num,
//                        (med_int)entities[i]->physicals.size(),
//                        groupName.c_str()) < 0)
////    #else
////            if(MEDfamCr(fid, meshName, (char *)familyName.c_str(), (med_int)num, 0,
////                        0, 0, 0, (char *)groupName.c_str(),
////                        (med_int)entities[i]->physicals.size()) < 0)
////    #endif
//              throw insight::Exception(
//                        str(boost::format("Could not create MED family %d") % num) );
//          }
//        }
      }

      // write the nodes
       {
         std::vector<med_float> coord;
         std::vector<med_int> fam;
         for (NodeIterator pi=nodeIt; pi!=NodeIterator(); ++pi)
         {
             coord.push_back( pi->X() );
             coord.push_back( pi->Y() );
             coord.push_back( pi->Z() );
             fam.push_back(0); // we never create node families
         }
//         for(unsigned int i = 0; i < entities.size(); i++) {
//           for(unsigned int j = 0; j < entities[i]->mesh_vertices.size(); j++) {
//             MVertex *v = entities[i]->mesh_vertices[j];
//             if(v->getIndex() >= 0) {
//               coord.push_back(v->x() * scalingFactor);
//               coord.push_back(v->y() * scalingFactor);
//               coord.push_back(v->z() * scalingFactor);
//               fam.push_back(0); // we never create node families
//             }
//           }
//         }
         if(fam.empty())
         {
           throw insight::Exception("No nodes to write in MED mesh");
         }

         if ( MEDmeshNodeWr(
                  fid,
                  meshName,
                  MED_NO_DT,
                  MED_NO_IT,
                  0.,
                  MED_FULL_INTERLACE,
                  (med_int)fam.size(),
                  &coord[0],
                  MED_FALSE, "",
                  MED_FALSE, 0,
                  MED_TRUE, &fam[0]
                  ) < 0)
         {
           throw insight::Exception("Could not write nodes");
         }
       }


//      // write the elements
//       {
//         { // points
//           med_geometrie_element typ = MED_NONE;
//           std::vector<med_int> conn, fam;
//           for(viter it = firstVertex(); it != lastVertex(); it++)
//             if(saveAll || (*it)->physicals.size())
//               fillElementsMED(families[*it], (*it)->points, conn, fam, typ);
//           writeElementsMED(fid, meshName, conn, fam, typ);
//         }
//         { // lines
//           med_geometrie_element typ = MED_NONE;
//           std::vector<med_int> conn, fam;
//           for(eiter it = firstEdge(); it != lastEdge(); it++)
//             if(saveAll || (*it)->physicals.size())
//               fillElementsMED(families[*it], (*it)->lines, conn, fam, typ);
//           writeElementsMED(fid, meshName, conn, fam, typ);
//         }
         { // triangles
           med_geometry_type typ = MED_NONE;
           std::vector<med_int> conn, fam;
//           for(fiter it = firstFace(); it != lastFace(); it++)
//             if(saveAll || (*it)->physicals.size())
//               fillElementsMED(families[*it], (*it)->triangles, conn, fam, typ);
           for (TriIterator j=triIt; j!=TriIterator(); ++j)
             {
               typ = MED_TRIA3;
               conn.push_back( j->Value(1) );
               conn.push_back( j->Value(2) );
               conn.push_back( j->Value(3) );
               fam.push_back(0);

//               for(unsigned int i = 0; i < elements.size(); i++)
//               {
//                 elements[i]->setVolumePositive();
//                 for(int j = 0; j < elements[i]->getNumVertices(); j++)
//                   conn.push_back(
//                     elements[i]->getVertex(med2mshNodeIndex(type, j))->getIndex());
//                 fam.push_back(family);
//               }
             }

//           writeElementsMED(fid, meshName, conn, fam, typ);
           {
             if (MEDmeshElementWr(
                         fid,
                         meshName,
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
           }

         }


         if(MEDfileClose(fid) < 0) {
           throw insight::Exception(
                         str(boost::format("Unable to close file '%s'") % fileName)
                         );
         }

}

class NodeIterator
{
    // Iterator tags here...
    using difference_type   = std::ptrdiff_t;
    using value_type        = gp_Pnt;
    using pointer           = gp_Pnt*;  // or also value_type*
    using reference         = gp_Pnt&;  // or also value_type&

    Handle_Poly_Triangulation pt_;
    Standard_Integer i_;

public:
    NodeIterator()
        : i_(-1)
    {}

    NodeIterator(Handle_Poly_Triangulation pt, Standard_Integer i=1)
        : pt_(pt), i_(i)
    {}

    const value_type& operator*() const { return pt_->Node(i_); }
    const value_type* operator->() const { return &pt_->Node(i_); }

    NodeIterator operator++(int)
    { NodeIterator tmp = *this; ++(*this); return tmp; }
    NodeIterator& operator++()
    {
        i_ = ( !pt_.IsNull() && (i_<pt_->NbNodes()) ) ? i_+1 : -1;
        return *this;
    }
    friend bool operator== (const NodeIterator& a, const NodeIterator& b)
    { return a.pt_ == b.pt_ && a.i_==b.i_; };
    friend bool operator!= (const NodeIterator& a, const NodeIterator& b)
    { return !operator==(a, b); };
};


class Poly_Triangle_Iterator
{
    // Iterator tags here...
    using difference_type   = std::ptrdiff_t;
    using value_type        = Poly_Triangle;
    using pointer           = Poly_Triangle*;  // or also value_type*
    using reference         = Poly_Triangle&;  // or also value_type&

    Handle_Poly_Triangulation pt_;
    Standard_Integer i_;

public:
    Poly_Triangle_Iterator()
        : i_(-1)
    {}

    Poly_Triangle_Iterator(Handle_Poly_Triangulation pt, Standard_Integer i=1)
        : pt_(pt), i_(i)
    {}

    const value_type& operator*() const { return pt_->Triangle(i_); }
    const value_type* operator->() const { return &pt_->Triangle(i_); }

    Poly_Triangle_Iterator operator++(int)
    { Poly_Triangle_Iterator tmp = *this; ++(*this); return tmp; }
    Poly_Triangle_Iterator& operator++()
    {
        i_ = ( !pt_.IsNull() && (i_<pt_->NbTriangles()) ) ? i_+1 : -1;
        return *this;
    }
    friend bool operator== (const Poly_Triangle_Iterator& a, const Poly_Triangle_Iterator& b)
    { return a.pt_ == b.pt_ && a.i_==b.i_; };
    friend bool operator!= (const Poly_Triangle_Iterator& a, const Poly_Triangle_Iterator& b)
    { return !operator==(a, b); };
};

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


defineType(FrameMesh);

size_t FrameMesh::calcHash() const
{
    return 0;
}

void FrameMesh::build()
{
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

            Poly_ListOfTriangulation triangulations;
            TopoDS_Shape s = xsec->shape();
            for (TopExp_Explorer ex(s, TopAbs_FACE); ex.More(); ex.Next())
            {
                auto f=TopoDS::Face(ex.Current());
                auto loc=f.Location();
                auto mesh = BRep_Tool::Triangulation(f,loc);

                std::cout<<"add mesh with nodes="<<mesh->NbNodes()<<" / faces="<<mesh->NbTriangles()<<std::endl;
                triangulations.Append(mesh);
            }

            auto entireMesh = Poly::Catenate(triangulations);
            std::cout<<"full mesh with nodes="<<entireMesh->NbNodes()<<" / faces="<<entireMesh->NbTriangles()<<std::endl;
            writeMED(
                    "test.med",
                    "tesgrup",
                    NodeIterator(entireMesh),
                    Poly_Triangle_Iterator(entireMesh)
                    );

        }
    }

    mesh.write();
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
