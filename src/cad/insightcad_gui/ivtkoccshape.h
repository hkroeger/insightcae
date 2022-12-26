#ifndef IVTKOCCSHAPE_H
#define IVTKOCCSHAPE_H

#include <limits>
#include "vtkPolyDataAlgorithm.h"
#include "TopoDS_Shape.hxx"
#include "base/vtkrendering.h"


vtkOStreamWrapper operator<<(vtkOStreamWrapper& os, const TopoDS_Shape& s);

class ivtkOCCShape : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(ivtkOCCShape,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static ivtkOCCShape *New();

  vtkSetMacro(Shape,TopoDS_Shape);
  vtkGetMacro(Shape,TopoDS_Shape);
  vtkSetMacro(Representation,insight::DatasetRepresentation);
  vtkGetMacro(Representation,insight::DatasetRepresentation);
  vtkSetMacro(MaxDeflection,double);
  vtkGetMacro(MaxDeflection,double);

protected:
  ivtkOCCShape();
  ~ivtkOCCShape();

  int FillOutputPortInformation( int port, vtkInformation* info ) override;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

private:
  ivtkOCCShape(const ivtkOCCShape&);  // Not implemented.
  void operator=(const ivtkOCCShape&);  // Not implemented.

  TopoDS_Shape Shape;
  insight::DatasetRepresentation Representation;
  double MaxDeflection;
};

#endif // IVTKOCCSHAPE_H
