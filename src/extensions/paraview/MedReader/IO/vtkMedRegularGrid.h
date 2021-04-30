// Copyright (C) 2010-2012  CEA/DEN, EDF R&D
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

#ifndef __vtkMedRegularGrid_h_
#define __vtkMedRegularGrid_h_

#include "vtkMedGrid.h"
#include "vtkMed.h"
#include "vtkMedSetGet.h"

#include <vector>

class vtkDataArray;

class VTK_EXPORT vtkMedRegularGrid : public vtkMedGrid
{
public :
  static vtkMedRegularGrid* New();
  vtkTypeMacro(vtkMedRegularGrid, vtkMedGrid)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Container of the families in this mesh
  vtkGetObjectVectorMacro(AxisCoordinate, vtkDataArray);
  vtkSetAbstractObjectVectorMacro(AxisCoordinate, vtkDataArray);

  // Description:
  // overloaded to set the number of coordinates arrays.
  // Do not allocate each array.
  virtual void  SetDimension(med_int);
  virtual int GetDimension();

  // Description:
  // the size of each dimension of the grid.
  // SetDimension has to have been called before
  virtual void  SetAxisSize(int axis, med_int size);
  virtual med_int GetAxisSize(int dim);

  // Description:
  // returns the number of points of this grid.
  virtual med_int GetNumberOfPoints();

  virtual void  LoadCoordinates();

  virtual double* GetCoordTuple(med_int index);

  virtual int IsCoordinatesLoaded();

  virtual vtkDataSet* CreateVTKDataSet(vtkMedFamilyOnEntityOnProfile*);

protected:
  vtkMedRegularGrid();
  virtual ~vtkMedRegularGrid();

  double CoordTuple[3];

  std::vector<med_int> AxisSize;
  //BTX
  vtkObjectVector<vtkDataArray>* AxisCoordinate;
  //ETX

private:
  vtkMedRegularGrid(const vtkMedRegularGrid&); // Not implemented.
  void operator=(const vtkMedRegularGrid&); // Not implemented.

};

#endif //__vtkMedRegularGrid_h_
