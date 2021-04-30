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

// Instantiate superclass first to give the template a DLL interface.
#include "vtkMed.h"

#ifndef WIN32
#include "vtkAOSDataArrayTemplate.txx"
//vtkCreateWrappedArrayInterface(med_int);

#include "vtkArrayIteratorTemplate.txx"
//VTK_ARRAY_ITERATOR_TEMPLATE_INSTANTIATE(med_int);
#endif

#define __vtkMedIntArrayInternal_cxx
#include "vtkMedIntArrayInternal.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMedIntArrayInternal);

//----------------------------------------------------------------------------
vtkMedIntArrayInternal::vtkMedIntArrayInternal(vtkIdType numComp) //: RealSuperclass(numComp)
{
  SetNumberOfComponents(numComp);
}

//----------------------------------------------------------------------------
vtkMedIntArrayInternal::~vtkMedIntArrayInternal()
{
}

//----------------------------------------------------------------------------
void vtkMedIntArrayInternal::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os,indent);
}
