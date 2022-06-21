#include "ivtkoccshape.h"

#include "vtkInformation.h"
#include "Standard_Version.hxx"
#include "TopoDS_Shape.hxx"
#include "Poly_Triangulation.hxx"
#include "BRep_Tool.hxx"
#include "TopExp_Explorer.hxx"
#include "TopoDS.hxx"
#include "TopoDS_Face.hxx"
#include "vtkCellArray.h"
#include "BRepMesh_FastDiscret.hxx"
#include "BRepMesh_IncrementalMesh.hxx"
#include "BRepTools.hxx"
#include "Bnd_Box.hxx"


vtkOStreamWrapper operator<<(vtkOStreamWrapper& os, const TopoDS_Shape& s)
{
    os << "TopoDS_Shape";
    return os;
}




vtkStandardNewMacro(ivtkOCCShape);

ivtkOCCShape::ivtkOCCShape()
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
  vtkNew<vtkCellArray> polys;

  for (TopExp_Explorer ex(Shape,TopAbs_FACE); ex.More(); ex.Next())
  {
      auto small = TopoDS::Face(ex.Current());

      TopLoc_Location L;
      auto tri = BRep_Tool::Triangulation (small, L);

      double deflection=1e-3;

      if (tri.IsNull()){
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

      long long i0=points->GetNumberOfPoints();

      for (vtkIdType i=0; i<tri->NbNodes(); ++i)
      {
          points->InsertNextPoint( tri->Node(i+1).XYZ().GetData() );
      }
      for (vtkIdType j=0; j<tri->NbTriangles(); ++j)
      {
          auto t = tri->Triangle(j+1);
          polys->InsertNextCell({
                   t.Value(1)+i0-1,
                   t.Value(2)+i0-1,
                   t.Value(3)+i0-1 });
      }
  }

  polydata->SetPoints(points.GetPointer());
  polydata->SetPolys(polys.GetPointer());

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

