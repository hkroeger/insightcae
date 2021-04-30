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

/*
 * TestMedParallelRead.cxx
 *
 *  Created on: 20 avr. 2011
 *      Author: alejandro
 */

#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define MED_HAVE_MPI

#include <vtkMed.h>
#define MESGERR 1
#include "med_utils.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

int main (int argc, char **argv)
{
  med_err _ret=0;
  med_idt _fid;

  int mpi_size, mpi_rank;
  MPI_Comm comm = MPI_COMM_WORLD;
  MPI_Info info = MPI_INFO_NULL;

  med_int    _nbofentitiesfiltered=0;
  med_int    *_filterarray=NULL;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

  med_size            _nbblocksperproc    = 0;
  int           _nbofentities             = 0;
  int           _nbofvaluesperentity      = 0;
  int           _nbofconstituentpervalue  = 0;

  printf("mpi_size = %03d\n", mpi_size);

  if (mpi_rank == 0 ) {

    struct tm *_tm ;
    time_t _tt=time(0);
    _tm = localtime(&_tt);

    srandom((*_tm).tm_sec * (*_tm).tm_min );
    _nbblocksperproc         = 1 + (int) (mpi_size * (random() / (RAND_MAX + 1.0)));
    _nbofentities            = 1 + (int) (1000.0 * (random() / (RAND_MAX + 1.0)));
    _nbofvaluesperentity     = 1 + (int) (11.0 * (random() / (RAND_MAX + 1.0)));
    _nbofconstituentpervalue = 1 + (int) (7.0 * (random() / (RAND_MAX + 1.0)));
  }

  MPI_Bcast(&_nbblocksperproc         , 1, MPI_LONG, 0, MPI_COMM_WORLD);
  MPI_Bcast(&_nbofentities            , 1, MPI_LONG, 0, MPI_COMM_WORLD);
  MPI_Bcast(&_nbofvaluesperentity     , 1, MPI_LONG, 0, MPI_COMM_WORLD);
  MPI_Bcast(&_nbofconstituentpervalue , 1, MPI_LONG, 0, MPI_COMM_WORLD);

  printf( "NENT-%03d_NVAL-%03d_NCST-%03d_NBL-%03llu\n",_nbofentities,_nbofvaluesperentity,
          _nbofconstituentpervalue,_nbblocksperproc);

  char         _filename   [255]="";
  sprintf(_filename,"/home/alejandro/work/Data-test-Med/tmp/depl.resu.med");
  /*     SSCRUTE(_filename); */

  /* Ouverture du fichier en mode parallel */
  if ((_fid = MEDparFileOpen(_filename, MED_ACC_RDONLY ,comm, info)) < 0){
    MED_ERR_(_ret,MED_ERR_OPEN,MED_ERR_FILE,_filename);
  }

  if ( MEDfileClose( _fid ) < 0) {
      MED_ERR_(_ret,MED_ERR_CLOSE,MED_ERR_FILE,""); _ret = -1;
    }

  /* MPI_Finalize must be called AFTER MEDclose which may use MPI calls */
  MPI_Finalize();

}
