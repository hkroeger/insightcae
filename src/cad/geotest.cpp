/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "geotest.h"
#include "base/exception.h"

#include <algorithm>

using namespace std;

namespace insight 
{
namespace cad 
{

double edgeLength(const TopoDS_Edge& e)
{
  GProp_GProps gpr;
  double l = 0.;
  if (!e.IsNull() && !BRep_Tool::Degenerated(e)) {
    BRepGProp::LinearProperties(e, gpr);
    l = gpr.Mass();
  }
  return l;
}

std::vector<gp_Pnt> resampleEdgeUniform(const TopoDS_Edge& edge, double approxSegmentLength, double* actualSegmentLength, int minSegments)
{
  double length=edgeLength(edge);
  int nSegments;

  if (approxSegmentLength > 0){
    nSegments=ceil(length/approxSegmentLength);
    nSegments = max(minSegments, nSegments);
  } else {
    throw insight::Exception("unable to split edge using a negative segment length");
  }

  if (actualSegmentLength){
    *actualSegmentLength = length/(double)(nSegments);
  }
  return resampleEdgeUniform(edge, nSegments);
}

std::vector<gp_Pnt> resampleEdgeUniform(const TopoDS_Edge& edge, int nSegments)
{
//  BRepAdaptor_Curve adap(edge);
//  nSegments = max(1, nSegments);
//  GCPnts_UniformAbscissa abscissa(adap, nSegments+1, 0.001);
//  if (!abscissa.IsDone())
//    throw PGError("Uniform discretization failed");
//  std::vector<gp_Pnt> tValues;
//  for (int i=1;i<=nSegments+1;i++){
//    //tValues.push_back(abscissa.Parameter(i));
//    gp_Pnt p;
//    BOPTools_Tools::PointOnEdge(edge, abscissa.Parameter(i), p);
//    tValues.push_back(p);
//  }
//  return tValues;
  std::vector<gp_Pnt> points;
  std::vector<double> tValues, uValues;

  for (int a=0; a<=nSegments; a++){
    double frac = (double)a/double(nSegments);
    tValues.push_back(frac);
  }
  uValues = resampleEdge(edge, tValues, 2.*nSegments);

  for (int a=0; a<(int)uValues.size(); a++){
    points.push_back(edgeAt(edge, uValues[a]));
  }
  return points;

}

/**
 * Returns a list of u-Values, where a u-Value corresponds to a lIn value. The sub-edge
 * 0..u has a length of lIn*length(c0).
 * To calculate the length, the edge c0 is Split up into c0Res sub-edges.
 */
std::vector<double> resampleEdge (const TopoDS_Edge& edge, const std::vector<double> lIn, int c0Res)
{
  double s;//, s0, s1;
  gp_Pnt p0, p1;
  //Handle_Geom_Curve c0 = BRep_Tool::Curve(edge, s0, s1);
  std::vector<double> uOut;
  if (c0Res < 1){
    c0Res = lIn.size()*2;
  }
  c0Res = max(1,c0Res);
  double l[c0Res+1], L=0;

  l[0] = 0.;
  p0 = edgeAt(edge, 0.); //c0->Value(s0);
  for(int i=1; i<=c0Res;i++){
    s = (double)(i)/(double)c0Res;
    p1 = edgeAt(edge, s); //c0->Value(s);
    L += p1.Coord().Subtracted(p0.Coord()).Modulus();
    l[i] = L;
    p0 = p1;
  }
  for(int i=1; i<=c0Res;i++){
    l[i] /= L;
  }

  for (unsigned int i=0; i<lIn.size(); i++){
    double li = lIn[i], u;
    int j;
    for (j=0; l[j+1]<li && j<c0Res; j++);
    u = (li-l[j])/(l[j+1]-l[j])*1./(double)c0Res + (double)j/(double)c0Res;
    uOut.push_back(u);
  }

  return uOut;
}

gp_Pnt2d faceUV(const TopoDS_Face& f, const TopoDS_Vertex& v)
{
  return faceUV(f, BRep_Tool::Pnt(v));
}

gp_Pnt2d faceUV(const TopoDS_Face& f, const gp_Pnt& p)
{
  return faceUV(BRep_Tool::Surface(f), p);
}

gp_Pnt2d faceUV(const Handle_Geom_Surface& f, const gp_Pnt& p)
{
  ShapeAnalysis_Surface sas(f);
  return sas.ValueOfUV(p, 1e-7);
}

std::vector<gp_XY> faceUV
(
    const TopoDS_Face& f,
    std::vector<gp_Pnt>::const_iterator begin,
    std::vector<gp_Pnt>::const_iterator end
)
{
  std::vector<gp_XY> res;
  for (std::vector<gp_Pnt>::const_iterator i=begin; i!=end; i++)
  {
    res.push_back(faceUV(f, *i).XY());
  }
  return res;
}

gp_Vec faceNormal(const TopoDS_Face& f, const gp_Pnt& v)
{
  return faceNormal(f, faceUV(f, v));
}

gp_Vec faceNormal(const TopoDS_Face& f, const TopoDS_Vertex& v)
{
  return faceNormal(f, faceUV(f, v));
}

gp_Vec faceNormal(const TopoDS_Face& f, const gp_Pnt2d& p2)
{
  gp_Vec n=GeomLProp_SLProps
      (
          BRep_Tool::Surface(f),
          p2.X(), p2.Y(),
          1, Precision::Confusion()
      ).Normal();
  n.Normalize();
  if (f.Orientation()==TopAbs_REVERSED){
    n.Multiply(-1.);
  }
  return n;
}


gp_Pnt faceAt(const TopoDS_Face& face, gp_Pnt2d p2d )
{
  return faceAt(face, p2d.X(),p2d.Y());
}

gp_Pnt faceAt(const TopoDS_Face& face, double u, double v )
{
  Handle_Geom_Surface surf_=BRep_Tool::Surface(face);
//  return surf_->Value(min(1., max(0., u)), min(1., max(0., v)));
  return surf_->Value(u, v);
}

std::vector<gp_Pnt> faceAt
(
    const TopoDS_Face& face,
    std::vector<gp_XY>::const_iterator begin,
    std::vector<gp_XY>::const_iterator end
)
{
  std::vector<gp_Pnt> res;
  for (std::vector<gp_XY>::const_iterator i=begin; i!=end; i++)
  {
    res.push_back(faceAt(face, i->X(), i->Y()));
  }
  return res;
}

gp_Pnt edgeAt(const TopoDS_Edge& edge, double t)
{
//  TopTools_IndexedMapOfShape vertexList;
//  TopExp::MapShapes(edge, TopAbs_VERTEX, vertexList);
//  if (vertexList.Extent()>0){

  double start, end;
  Handle_Geom_Curve c0 = BRep_Tool::Curve(edge, start, end);

  if (edgeLength(edge)>0){
    t = max(t,0.);
    t = min(t,1.);
    double start, end;
    Handle_Geom_Curve c = BRep_Tool::Curve(edge, start, end);
    if (edge.Orientation()==TopAbs_REVERSED){
      t = 1.-t;
    }
    return c->Value(t*(end-start)+start);
  } else {
    TopExp_Explorer ex(edge,TopAbs_VERTEX);
    if (ex.More()){
      gp_Pnt pnt(BRep_Tool::Pnt(TopoDS::Vertex(ex.Current())));
      return pnt;
    } else {
      return gp_Pnt(0.,0.,0.);

    }
  }
//  }
//  throw( PGError("edgeAt: Given edge is undefined."));
//  return gp_Pnt(0,0,0);
}

Bnd_Box getBoundingBox(const TopoDS_Shape& shape, double deflection)
{

    if (deflection>0){
        Bnd_Box box;
        BRepMesh_FastDiscret m(deflection, 0.5, box, true, false, false, false);
        m.Perform(shape);
        //    BRepMesh_IncrementalMesh Inc(shape, deflection);
    }


    Bnd_Box bounds;
    BRepBndLib::Add(shape, bounds);
    return bounds;
}

Bnd_Box getBoundingBox(const TopoDS_Shape& shape, gp_Pnt& bbMin, gp_Pnt& bbMax, double deflection )
{
  Bnd_Box bounds;
  double ext[6];
  bounds = getBoundingBox(shape, deflection);
  bounds.Get(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);

  bbMin.SetCoord(ext[0],ext[1],ext[2]);
  bbMax.SetCoord(ext[3],ext[4],ext[5]);

  return bounds;
}

  
//BoundingBox Test
bool isPartOf( const Bnd_Box& big, const Bnd_Box& small, double tolerance )
{
    double bigExt[6],smallExt[6];
    double diag = sqrt( big.SquareExtent() );
    Bnd_Box bigger( big );
    bigger.Enlarge(diag*0.2);
    bigger.Get(bigExt[0], bigExt[1], bigExt[2], bigExt[3], bigExt[4], bigExt[5]);
    small.Get(smallExt[0], smallExt[1], smallExt[2], smallExt[3], smallExt[4], smallExt[5]);

    if (smallExt[0]+tolerance >= bigExt[0] &&
        smallExt[1]+tolerance >= bigExt[1] &&
        smallExt[2]+tolerance >= bigExt[2] &&
        smallExt[3] <= bigExt[3]+tolerance &&
        smallExt[4] <= bigExt[4]+tolerance &&
        smallExt[5] <= bigExt[5]+tolerance ) {
        return true;
    } else {
        return false;
    }
}

//EdgeTest
bool isPartOf(const TopoDS_Edge& big, const TopoDS_Vertex& p, double tolerance)
{
  gp_Pnt pOnCurve;
  ShapeAnalysis_Curve sac;

  double u,start, end,
         dist = sac.Project(BRep_Tool::Curve(big,start, end), BRep_Tool::Pnt(p), tolerance, pOnCurve, u,true);
  return (dist <= tolerance);
}

bool isPartOf(const TopoDS_Edge& big, const TopoDS_Edge& e, double tolerance, int nSamples)
{
  bool ok = isPartOf(getBoundingBox(big), getBoundingBox(e, tolerance*0.5), tolerance);

  if (ok) {
      //Anfangs- und Endpunkt prüfen:
      TopExp_Explorer ex(e,TopAbs_VERTEX);
      while (ex.More() && ok){
          TopoDS_Vertex v = TopoDS::Vertex(ex.Current());
          ok = isPartOf(big, v, tolerance);
          ex.Next();
      }
      if (ok) {
          std::vector<gp_Pnt> samples = resampleEdgeUniform(e, nSamples);
          for (int i=0; i<(int)samples.size() && ok; i++){
              ok = isPartOf(big, BRepBuilderAPI_MakeVertex(samples[i]), tolerance);
          }
      }
  }
  return ok;
}

//FaceTest
bool isPartOf(const TopoDS_Face& big, const TopoDS_Vertex& p, double tolerance)
{
    gp_Pnt pnt(BRep_Tool::Pnt(p));
    gp_Pnt2d pnt2D(faceUV(big, pnt));
    double d = faceAt(big, pnt2D).Coord().Subtracted(pnt.Coord()).SquareModulus();
    return (d <= tolerance*tolerance);
}

bool isPartOf(const TopoDS_Face& big, const TopoDS_Edge& e, double tolerance, int nSamples)
{
    bool ok = isPartOf(getBoundingBox(big), getBoundingBox(e, tolerance*0.5), tolerance);

    if (ok) {
        //Anfangs- und Endpunkt prüfen:
        TopExp_Explorer ex(e,TopAbs_VERTEX);
        while (ex.More() && ok){
            TopoDS_Vertex v = TopoDS::Vertex(ex.Current());
            ok = isPartOf(big, v, tolerance);
            ex.Next();
        }
        if (ok) {
            std::vector<gp_Pnt> samples = resampleEdgeUniform(e, nSamples);
            for (int i=0; i<(int)samples.size() && ok; i++){
                ok = isPartOf(big, BRepBuilderAPI_MakeVertex(samples[i]), tolerance);
            }
        }
    }
    return ok;
}

bool isPartOf(const TopoDS_Face& big, const TopoDS_Face& small, double tolerance, int nSamples)
{

  bool ok = isPartOf(getBoundingBox(big), getBoundingBox(small));

    if (ok){
        // Prüfen, ob die Kanten von small Teil der Fläche sind.
        TopExp_Explorer ex(small,TopAbs_EDGE);
        while (ex.More() && ok){
            TopoDS_Edge e = TopoDS::Edge(ex.Current());
            ok = isPartOf(big, e, tolerance, nSamples);
            ex.Next();
        }
        // Testpunkte prüfen
        if (ok) {
          TopLoc_Location L;
          Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation (small, L);
          if (tri.IsNull()){
            Bnd_Box box;
            box = getBoundingBox(small);
            tri = BRep_Tool::Triangulation (small, L);
            if (tri.IsNull()){
              BRepTools::Clean(small);
              BRep_Builder b;
              b.UpdateFace(small, tolerance);
              BRepMesh_FastDiscret m(0.1, 0.5, box, true, true, false, false);

//              m.UpdateFace(small, TNull)

//              BRepMesh_FastDiscret m(0.001, small, box, 0.5, false, false, true, false);
//              m.result();
//              int cou = m.NbTriangles();
//              m.Add(small);
//              m.Process(small);
              m.Perform(small);
//              int counter = m.NbVertices();
              tri = BRep_Tool::Triangulation (small, L);
            }
          }
          if (tri.IsNull()){
            ShapeFix_Face fix(small);
            fix.Perform();
            TopoDS_Face f = TopoDS::Face(fix.Result());
            BRepTools::Clean(f);
            TopLoc_Location L;
            //BRepMesh::Mesh(f, 0.3);
            BRepMesh_IncrementalMesh(f, 0.3);
            tri = BRep_Tool::Triangulation(f, L);
          }
          if (!tri.IsNull()){
            int nbNodes = tri->NbNodes();
            int delta = nbNodes / nSamples;
            delta = std::max(delta, 1);
            for (int i=1; i<tri->NbNodes(); i+=delta){
              const TColgp_Array1OfPnt& nodes = tri->Nodes();
              gp_XYZ p = nodes(i).Coord();
              L.Transformation().Transforms(p);
              ok = isPartOf(big, BRepBuilderAPI_MakeVertex(gp_Pnt(p)), tolerance);
            }
          } else {
            ok = false;
          }
        }
    }
    return ok;
}

//ShellTest
bool isPartOf(const TopoDS_Shell& big, const TopoDS_Edge& small, double tolerance, int nSamples)
{
    bool ok = isPartOf(getBoundingBox(big), getBoundingBox(small));
    bool found = false;

    if (ok){
        TopExp_Explorer ex(big,TopAbs_FACE);
        while (ex.More() && !found){
            TopoDS_Face face = TopoDS::Face(ex.Current());
            found = isPartOf(face, small, tolerance, nSamples);
            ex.Next();
        }
    }
    return found;
}

bool isPartOf(const TopoDS_Shell& big, const TopoDS_Face& small, double tolerance, int nSamples)
{
    bool ok = isPartOf(getBoundingBox(big), getBoundingBox(small));
    bool found = false;

    if (ok){
        // Prüfen, ob das Face small Teil von big sind.
        TopExp_Explorer ex(big,TopAbs_FACE);
        while (ex.More() && !found){
            TopoDS_Face face = TopoDS::Face(ex.Current());
            found = isPartOf(face, small, tolerance, nSamples);
            ex.Next();
        }
    }
    return found;
}

//SolidTest
bool isPartOf(const TopoDS_Solid& big, const TopoDS_Shape& small, double tolerance, int nSamples)
{
    bool ok = isPartOf(getBoundingBox(big), getBoundingBox(small));
    bool found = false;

    if (ok){
        // Prüfen, ob das Face small Teil von big sind.
        TopExp_Explorer ex(big,TopAbs_SHELL);
        while (ex.More() && !found){
            TopoDS_Shell shell = TopoDS::Shell(ex.Current());
            found = isPartOf(shell, small, tolerance, nSamples);
            ex.Next();
        }
    }
    return found;
}


//Allgemein
bool isPartOf(const TopoDS_Shape& big, const TopoDS_Shape& small, double tolerance, int nSamples)
{
  bool ok = false;
  if (!big.IsNull() && !small.IsNull()){

    TopAbs_ShapeEnum bigType = big.ShapeType();
    if (bigType == TopAbs_SOLID) {
      //Anstatt des Solids werden seine Shells untersucht:
      TopoDS_Solid solid = TopoDS::Solid(big);
       ok = isPartOf(solid, small, tolerance, nSamples);
    } else if (bigType == TopAbs_SHELL) {
      TopoDS_Shell BIG = TopoDS::Shell(big);
      if (small.ShapeType() == TopAbs_FACE){
        TopoDS_Face SMALL = TopoDS::Face(small);
        ok = isPartOf(BIG, SMALL, tolerance, nSamples);
      } else if (small.ShapeType() == TopAbs_EDGE) {
        TopoDS_Edge SMALL = TopoDS::Edge(small);
        ok = isPartOf(BIG, SMALL, tolerance, nSamples);
      }
    } else if (bigType == TopAbs_FACE) {
      TopoDS_Face BIG = TopoDS::Face(big);

      if (small.ShapeType() == TopAbs_FACE){
        TopoDS_Face SMALL = TopoDS::Face(small);
        ok = isPartOf(BIG, SMALL, tolerance, nSamples);
      } else if (small.ShapeType() == TopAbs_EDGE) {
        TopoDS_Edge SMALL = TopoDS::Edge(small);
        ok = isPartOf(BIG, SMALL, tolerance, nSamples);
      } else if (small.ShapeType() == TopAbs_VERTEX) {
        TopoDS_Vertex SMALL = TopoDS::Vertex(small);
        ok = isPartOf(BIG, SMALL, tolerance, nSamples);
      }
    } else if (bigType == TopAbs_EDGE) {
      TopoDS_Edge BIG = TopoDS::Edge(big);

      if (small.ShapeType() == TopAbs_EDGE){
        TopoDS_Edge SMALL = TopoDS::Edge(small);
        ok = isPartOf(BIG, SMALL, tolerance, nSamples);
      } if (small.ShapeType() == TopAbs_VERTEX){
        TopoDS_Vertex SMALL = TopoDS::Vertex(small);
        ok = isPartOf(BIG, SMALL, tolerance);
      }

    }
  }
  return ok;
}



  
bool isEqual
(
    const TopoDS_Edge& e1,
    const TopoDS_Edge& e2,
    double tol
)
{
  gp_Pnt ps1=BRep_Tool::Pnt(TopExp::FirstVertex(e1));
  gp_Pnt pe1=BRep_Tool::Pnt(TopExp::LastVertex(e1));
  gp_Pnt ps2=BRep_Tool::Pnt(TopExp::FirstVertex(e2));
  gp_Pnt pe2=BRep_Tool::Pnt(TopExp::LastVertex(e2));

  if (
      ( (ps1.Distance(ps2)<tol) && (pe1.Distance(pe2)<tol ) )
      ||
      ( (ps1.Distance(pe2)<tol) && (pe1.Distance(ps2)<tol ) )
      )
    return true;
  else
    return false;

}

 
bool isEqual
(
    const TopoDS_Face& f1,
    const TopoDS_Face& f2,
    double etol
)
{
  bool foundAll=true;
  for (TopExp_Explorer ex(f1, TopAbs_EDGE); ex.More(); ex.Next())
  {
    bool foundCur=false;
    for (TopExp_Explorer ex2(f2, TopAbs_EDGE); ex2.More(); ex2.Next())
    {
      if (isEqual(TopoDS::Edge(ex.Current()), TopoDS::Edge(ex2.Current()), etol))
      {
        foundCur=true;
        break;
      }
    }
    if (!foundCur)
    {
      foundAll=false;
      break;
    }
  }
  return foundAll;
}

  
}
}
