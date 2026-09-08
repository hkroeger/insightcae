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
#include "cadexception.h"
#include "cadfeature.h"
#include "datum.h"
#include "BRepClass_FaceClassifier.hxx"
#include "cadfeatures/importsolidmodel.h"

#include <algorithm>

using namespace std;

namespace insight 
{
namespace cad 
{


  
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
//    gp_Pnt2d pnt2D(faceUV(big, pnt));
//    double d = faceAt(big, pnt2D).Coord().Subtracted(pnt.Coord()).SquareModulus();
//    return (d <= tolerance*tolerance);
    BRepClass_FaceClassifier fc(big, pnt, tolerance);
    return (fc.State() == TopAbs_IN) || (fc.State() == TopAbs_ON);
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
#if (OCC_VERSION_MAJOR>=7 && OCC_VERSION_MINOR>=4)
              IMeshTools_Parameters p;
              p.Angle=0.5;
              p.Deflection=0.1;
              p.Relative=false;
              BRepMesh_IncrementalMesh  m(small, p);
#else
#if (OCC_VERSION_MAJOR>=7)
              BRepMesh_FastDiscret::Parameters p;
              p.Angle=0.5;
              p.Deflection=0.1;
              p.Relative=false;    
              BRepMesh_FastDiscret m(box, p);
#else
              BRepMesh_FastDiscret m(0.1, 0.5, box, true, true, false, false);
#endif
//              m.UpdateFace(small, TNull)

//              BRepMesh_FastDiscret m(0.001, small, box, 0.5, false, false, true, false);
//              m.result();
//              int cou = m.NbTriangles();
//              m.Add(small);
//              m.Process(small);
              m.Perform(small);
//              int counter = m.NbVertices();
#endif
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


bool isEmptyShape(const TopoDS_Shape &s)
{
    return !TopExp_Explorer(s, TopAbs_VERTEX).More();
}


bool isGeometricallyIdentical(const TopoDS_Shape &s1, const TopoDS_Shape &s2)
{
    bool e1=isEmptyShape(s1);
    bool e2=isEmptyShape(s2);

    if (e1&&e2) return true;
    if (e1!=e2) return false;

    BRepAlgoAPI_Cut cutter(s1, s2);
    cutter.Build();

    if (!cutter.IsDone())
    {
        throw insight::CADException(
            {
             { "shape 1", cad::Import::create(s1) },
             { "shape 2", cad::Import::create(s2) }
            },
            "could not perform cut operation."
            );
    }

    bool emptyResult = isEmptyShape(cutter.Shape());
    return emptyResult;
}


  
}
}
