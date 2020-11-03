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

#include "vtkMedFilter.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMedFilter)

vtkMedFilter::vtkMedFilter()
{
  _start = 0;
  _stride = 0;
  _count = 0;
  _blocksize = 0;
  _lastblocksize = 0;
}

vtkMedFilter::~vtkMedFilter()
{
}

void vtkMedFilter::SetFilterSizes( int start, int stride,
    int count, int blocksize, int lastblocksize )
{
  _start = start;
  _stride = stride;
  _count = count;
  _blocksize = blocksize;
  _lastblocksize = lastblocksize;
}

void vtkMedFilter::GetFilterSizes( int& start, int& stride,
    int& count, int& blocksize, int& lastblocksize )
{
  start = _start;
  stride = _stride;
  count = _count;
  blocksize = _blocksize;
  lastblocksize = _lastblocksize;
}

void vtkMedFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
