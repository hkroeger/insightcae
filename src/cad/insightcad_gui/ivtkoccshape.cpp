#include "ivtkoccshape.h"

#include "vtkInformation.h"
#include "Standard_Version.hxx"
#include "TopoDS_Shape.hxx"
#include "Poly_Triangulation.hxx"
#include "BRep_Tool.hxx"
#include "TopExp_Explorer.hxx"
#include "TopoDS.hxx"
#include "TopoDS_Edge.hxx"
#include "TopoDS_Face.hxx"
#include "TopoDS_Vertex.hxx"
#include "vtkCellArray.h"
#include "BRepMesh_FastDiscret.hxx"
#include "BRepMesh_IncrementalMesh.hxx"
#include "BRepTools.hxx"
#include "Bnd_Box.hxx"
#include "GCPnts_QuasiUniformDeflection.hxx"
#include "BRepAdaptor_Curve.hxx"


vtkOStreamWrapper operator<<(vtkOStreamWrapper& os, const TopoDS_Shape& s)
{
    os << "TopoDS_Shape";
    return os;
}




vtkStandardNewMacro(ivtkOCCShape);

ivtkOCCShape::ivtkOCCShape()
    : Representation(insight::DatasetRepresentation::Surface),
      MaxDeflection(1e-3)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

ivtkOCCShape::~ivtkOCCShape()
{}

// This override is not needed as the FillOutputPortInformation on vtkPolyDataAlgorithm
// does this.  Override this if you need something different from one output that
// is a vtkPolyData
int ivtkOCCShape::FillOutputPortInformation( int port, vtkInformation* info )
{
  if ( port == 0 )
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData" );

    return 1;
  }

  return 0;
}

int ivtkOCCShape::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{

  // Get the output
  vtkPolyData *output = vtkPolyData::GetData(outputVector,0);

  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> polys, lines, verts;

  if (Representation == insight::DatasetRepresentation::Surface)
  {
      for (TopExp_Explorer ex(Shape,TopAbs_FACE); ex.More(); ex.Next())
      {
          auto small = TopoDS::Face(ex.Current());

          TopLoc_Location L;
          auto tri = BRep_Tool::Triangulation (small, L);

          double deflection=1e-3;

          if (tri.IsNull())
          {
            BRepTools::Clean(small);
            Bnd_Box box;
    //        BRep_Builder b;
    //        b.UpdateFace(small, tolerance);
    #if (OCC_VERSION_MAJOR>=7 && OCC_VERSION_MINOR>=4)
            IMeshTools_Parameters p;
            p.Angle=0.5;
            p.Deflection=deflection;
            p.Relative=false;
            BRepMesh_IncrementalMesh  m(small, p);
    #else
    #if (OCC_VERSION_MAJOR>=7)
            BRepMesh_FastDiscret::Parameters p;
            p.Angle=0.5;
            p.Deflection=deflection;
            p.Relative=false;
            BRepMesh_FastDiscret m(box, p);
    #else
            BRepMesh_FastDiscret m(deflection, 0.5, box, true, true, false, false);
    #endif
            m.Perform(small);
    #endif
            tri = BRep_Tool::Triangulation (small, L);
          }

          if (!tri.IsNull())
          {
              vtkIdType i0=points->GetNumberOfPoints();

              for (vtkIdType i=0; i<tri->NbNodes(); ++i)
              {
                  points->InsertNextPoint( tri->Nodes().Value(i+1).Transformed(L).XYZ().GetData() );
              }
              for (vtkIdType j=0; j<tri->NbTriangles(); ++j)
              {
                  auto t = tri->Triangles().Value(j+1);
                  vtkIdType c[3]={
                      t.Value(1)+i0-1,
                      t.Value(2)+i0-1,
                      t.Value(3)+i0-1 };
                  polys->InsertNextCell(3, c);
              }
          }
      }
  }

  if ( (polys->GetNumberOfCells()==0) // wireframe or edges only
       &&
       !(Representation==insight::DatasetRepresentation::Points) )
  {
      for (TopExp_Explorer ex(Shape, TopAbs_EDGE); ex.More(); ex.Next())
      {
          auto edge = TopoDS::Edge(ex.Current());
          GCPnts_QuasiUniformDeflection algo(BRepAdaptor_Curve(edge), MaxDeflection);
          int np=algo.NbPoints();
          vtkIdType pts[np];
          vtkIdType i0=points->GetNumberOfPoints();
          for (int i=0; i<np; ++i)
          {
              auto cp = algo.Value(i+1);
              points->InsertNextPoint(cp.X(), cp.Y(), cp.Z());
              pts[i]=i0+i;
          }
          lines->InsertNextCell(np, pts);
      }
  }

  if (Representation==insight::DatasetRepresentation::Points)
  {
      for (TopExp_Explorer ex(Shape, TopAbs_VERTEX); ex.More(); ex.Next())
      {
          auto v = TopoDS::Vertex(ex.Current());
          auto cp = BRep_Tool::Pnt(v);
          vtkIdType i0 = points->GetNumberOfPoints();
          verts->InsertNextCell(1, &i0 );
          points->InsertNextPoint(cp.X(), cp.Y(), cp.Z());
      }
  }

  polydata->SetPoints(points.GetPointer());
  polydata->SetPolys(polys.GetPointer());
  polydata->SetLines(lines.GetPointer());
  polydata->SetVerts(verts.GetPointer());

  //output = polydata.GetPointer(); //doesn't work
  output->ShallowCopy(polydata.GetPointer());

  return 1;
}


//----------------------------------------------------------------------------
void ivtkOCCShape::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Shape: "
      << ( this->Shape.IsNull() ? "(none)" : "TopoDS_Shape" ) << "\n";
}

