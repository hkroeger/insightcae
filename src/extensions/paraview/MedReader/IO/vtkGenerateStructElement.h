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

#ifndef __vtkGenerateStructElement_h__
#define __vtkGenerateStructElement_h__

#include "vtkUnstructuredGridAlgorithm.h"

class vtkMedStructElement;
class vtkGenerateStructElementCache;

class VTK_EXPORT vtkGenerateStructElement : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkGenerateStructElement* New();
  vtkTypeMacro(vtkGenerateStructElement, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

protected :
  vtkGenerateStructElement();
  virtual ~vtkGenerateStructElement();

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  //BTX
  virtual double  GetParameter1(const char*,
                           vtkIdType,
                           vtkGenerateStructElementCache&);
  //ETX

private :
  vtkGenerateStructElement(const vtkGenerateStructElement&);// Not implemented.
  void operator=(const vtkGenerateStructElement&); // Not implemented.

};

#endif //__vtkGenerateStructElement_h__
