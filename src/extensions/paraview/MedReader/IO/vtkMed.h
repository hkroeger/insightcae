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

#ifndef __vtkMed_h__
#define __vtkMed_h__

#include "hdf5.h"
#include "config.h"

extern "C"
{
#include "med.h"
#define MESGERR 1
#include "med_utils.h"

#ifdef MED_HAVE_MPI
#ifdef MedReader_BUILD_PARALLEL
#define MedReader_HAVE_PARALLEL_INFRASTRUCTURE
#endif
#endif

//#define MED_HAVE_MPI 1
//#include "mpi.h"
#ifdef MedReader_HAVE_PARALLEL_INFRASTRUCTURE
#include "mpi.h"
#endif
}

#endif //__vtkMed_h__
