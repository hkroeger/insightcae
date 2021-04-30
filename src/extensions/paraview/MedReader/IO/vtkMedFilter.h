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

#ifndef __vtkMedFilter_h_
#define __vtkMedFilter_h_

#include "vtkObject.h"
#include"vtkMed.h"

class VTK_EXPORT vtkMedFilter : public vtkObject
{
public:
  static vtkMedFilter* New();
  vtkTypeMacro(vtkMedFilter, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetFilterSizes( int , int , int , int , int );

  void GetFilterSizes( int& , int& , int& , int& , int& );

protected :
  vtkMedFilter();
  ~vtkMedFilter();

  //med_filter Filter;
    int  _start;
    int  _stride;
    int  _count;
    int  _blocksize;
    int  _lastblocksize;
};

#endif //__vtkMedFilter_h_
