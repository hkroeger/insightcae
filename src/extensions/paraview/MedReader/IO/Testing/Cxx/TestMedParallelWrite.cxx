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
 * TestMedParallelWrite.cxx
 *
 *  Created on: 11 mai 2011
 *      Author: alejandro
 */

#define MED_HAVE_MPI

#include <vtkMed.h>
#define MESGERR 1
#include "med_utils.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main (int argc, char **argv)
{
  const char meshname[MED_NAME_SIZE+1] = "2D unstructured mesh";
  const med_int spacedim = 2;
  const med_int meshdim = 2;
  /*                                         12345678901234561234567890123456 */
  const char axisname[2*MED_SNAME_SIZE+1] = "x               y               ";
  const char unitname[2*MED_SNAME_SIZE+1] = "cm              cm              ";
  med_float coordinates[2222];
  const med_int nnodes = 1111;

  med_int* quadconnectivity;
  const med_int nquad4 = 1000;

  med_err _ret=-1;

  int mpi_size, mpi_rank;
  MPI_Comm comm = MPI_COMM_WORLD;
  MPI_Info info = MPI_INFO_NULL;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

  med_idt  fid;
  char    filename[255]="UsesCase_MEDmesh_parallel.med";
  /*     SSCRUTE(_filename); */

  if (mpi_rank == 0 ) {
    printf("mpi_size = %03d\n", mpi_size);

    /* Ouverture du fichier en mode non-parallel */
    if ((fid = MEDfileOpen(filename, MED_ACC_CREAT)) < 0){
      MED_ERR_(_ret, MED_ERR_OPEN,MED_ERR_FILE, filename);
    }

    /* write a comment in the file */
    if (MEDfileCommentWr(fid,"A 2D unstructured mesh : 15 nodes, 12 cells") < 0) {
      MESSAGE("ERROR : write file description ...");
    }

    /* mesh creation : a 2D unstructured mesh */
    if (MEDmeshCr(fid, meshname, spacedim, meshdim, MED_UNSTRUCTURED_MESH,
      "A 2D unstructured mesh","",MED_SORT_DTIT,MED_CARTESIAN, axisname, unitname) < 0) {
      MESSAGE("ERROR : mesh creation ...");
    }

    /*
     * Building the coordinates of a rectangle of 101 points in the Y-axis,
     * and 11 in the X-axis
     */
    for (int j=0; j<11; j++ )
      for (int i=0; i<101; i++ )
      {
      coordinates[j*202+i*2]   = j+1;
      coordinates[j*202+i*2+1] = i+1;
      }

    /* nodes coordinates in a Cartesian axis in full interlace mode
        (X1,Y1, X2,Y2, X3,Y3, ...) with no iteration and computation step
     */
    if (MEDmeshNodeCoordinateWr(fid, meshname, MED_NO_DT, MED_NO_IT, 0.0,
              MED_FULL_INTERLACE, nnodes, coordinates) < 0) {
      MESSAGE("ERROR : nodes coordinates ...");
    }

    if ( MEDfileClose( fid ) < 0) {
      MED_ERR_(_ret,MED_ERR_CLOSE,MED_ERR_FILE,"");;
    }

    MPI_Barrier(comm);
  } /* End of process ZERO */
  else
    {
    MPI_Barrier(comm);
    }

  /* Ouverture du fichier en mode parallel */
  if ((fid = MEDparFileOpen(filename, MED_ACC_RDWR ,comm, info)) < 0){
    MED_ERR_(_ret, MED_ERR_OPEN,MED_ERR_FILE, filename);
  }

  med_int     nbofentity = nquad4;
  med_int     nbofvaluesperentity = 1;
  med_int     nbofconstituentpervalue = 4;
  med_int     constituentselect = MED_ALL_CONSTITUENT;
  med_switch_mode   switchmode = MED_FULL_INTERLACE;
  med_storage_mode    storagemode = MED_COMPACT_STMODE;
  const char *const   profilename = MED_NO_PROFILE;

  /*
   * Calculating block sizes
   */

  int block_size = (100/mpi_size)*10;
  med_size    start  = block_size * mpi_rank + 1;
  med_size    stride = block_size;
  med_size    count  = 1;
  med_size    blocksize = block_size;
  med_size    lastblocksize = (100 % mpi_size)*10;
  if ((mpi_size == mpi_rank+1) && (lastblocksize != 0))
    {
    blocksize += lastblocksize;
    stride    += lastblocksize;
    }
  lastblocksize = 0;

  printf("%03d: block_size = %03d\n", mpi_rank, block_size);
  printf("%03d: start = %03d\n", mpi_rank, start);
  printf("%03d: stride = %03d\n", mpi_rank, stride);
  printf("%03d: count = %03d\n", mpi_rank, count);
  printf("%03d: blocksize = %03d\n", mpi_rank, blocksize);
  printf("%03d: lastblocksize = %03d\n", mpi_rank, lastblocksize);
  med_filter filter = MED_FILTER_INIT;

  if ( MEDfilterBlockOfEntityCr( fid,
      nbofentity,
      nbofvaluesperentity,
      nbofconstituentpervalue,
      constituentselect,
      switchmode,
      storagemode,
      profilename,
      start,
      stride,
      count,
      blocksize,
      lastblocksize,
      &filter ) < 0 )
    {
    MESSAGE("ERROR : filter creation ...");
    }

  // Attention: there is blocksize and block_size and it does not
  // represent the same quantity, in case we are in the last
  // block they are different, if not it is the same
  quadconnectivity = new med_int[blocksize*4];
  int shift = mpi_rank*block_size;
  printf("%03d: mpi_rank*block_size = %03d\n", mpi_rank, shift);
  printf("%03d: block_size = %03d\n", mpi_rank, block_size);
  int base = shift + shift / 101;
  int c = 0;
  for (int i=0; i<blocksize*4; i+=4 )
    {
    base++;
    if ((base%101) == 0)
      base++;

    quadconnectivity[i]   = base;
    quadconnectivity[i+1] = base+1;
    quadconnectivity[i+2] = base+102;
    quadconnectivity[i+3] = base+101;
    c++;
    }
  printf("%03d: number of written quads = %03d\n", mpi_rank, c);

  if (MEDmeshElementConnectivityAdvancedWr(fid, meshname, MED_NO_DT,
           MED_NO_IT, 0.0, MED_CELL, MED_QUAD4,
           MED_NODAL, &filter, quadconnectivity) < 0) {
    MESSAGE("ERROR : quadrangular cells connectivity ...");
  }

    if ( MEDfilterClose( &filter ) < 0) {
      MESSAGE("ERROR : filter closing ...");
    }

    if ( MEDfileClose( fid ) < 0) {
      MED_ERR_(_ret,MED_ERR_CLOSE,MED_ERR_FILE,""); _ret = -1;
    }

    /* Barrier before writing family ZERO */
    MPI_Barrier(comm);

    if (mpi_rank == 0 ) {

      if ((fid = MEDfileOpen(filename, MED_ACC_RDWR)) < 0){
        MED_ERR_(_ret, MED_ERR_OPEN,MED_ERR_FILE, filename);
      }

      /* create family 0 : by default, all mesh entities family number is 0 */
      if (MEDfamilyCr(fid, meshname,MED_NO_NAME, 0, 0, MED_NO_GROUP) < 0) {
        MESSAGE("ERROR : family 0 creation ...");
      }

      const char familyname_root[MED_NAME_SIZE+1] = "PROCESSOR ";
      char familyname[MED_NAME_SIZE+1] = " ";
      for (int i=1; i<mpi_size+1; i++)
        {
        snprintf(familyname, sizeof familyname, "%s%d", familyname_root, i);
        if (MEDfamilyCr(fid, meshname,familyname, -i, 0, MED_NO_GROUP) < 0) {
          MESSAGE("ERROR : family creation ...");
          }
        printf("%03d: %s\n", mpi_rank, familyname);
        }

      med_int familynumbers[nquad4];
      int l = 1;
      for (int i=0; i<nquad4; i++)
        {
        if ((i > block_size * l - 1) && (l < mpi_size))
          {
          l++;
          }
        familynumbers[i] = -l;
        }

      if (MEDmeshEntityFamilyNumberWr(fid, meshname, MED_NO_DT, MED_NO_IT,
                           MED_CELL, MED_QUAD4, nquad4, familynumbers) < 0) {
        MESSAGE("ERROR : nodes family numbers ...");
      }

      /* Write a Profile */
      const char profileName[MED_NAME_SIZE+1] = "QUAD4_PROFILE";
      const med_int profilesize = 9;
      med_int profilearray[9] = {1, 3, 5, 7, 9, 11, 13, 15, 17};
      if (MEDprofileWr(fid, profileName, profilesize, profilearray ) < 0) {
        MESSAGE("ERROR : nodes family numbers ...");
      }

      /* write localization for integration points */
      const char localizationName[MED_NAME_SIZE+1] = "QUAD4_INTEGRATION_POINTS_4";
      const med_float elementcoordinate[6] = {0.0, 0.0,  1.0, 0.0,  0.0,1.0};
      const med_float iPointCoordinate[8] = {1.0/5, 1.0/5,  3.0/5, 1.0/5,  1.0/5, 3.0/5,  1.0/3, 1.0/3};
      const med_float weight[4] = {1.0/8, 1.0/8, 1.0/8, 1.0/8};
      med_int spacedim = 2;
      med_int nipoint = 4;
      if (MEDlocalizationWr(fid, localizationName, MED_QUAD4, spacedim,
          elementcoordinate, MED_FULL_INTERLACE,
          nipoint, iPointCoordinate, weight,
          MED_NO_INTERPOLATION, MED_NO_MESH_SUPPORT) < 0) {
        MESSAGE("ERROR : create family of integration points ...");
      }

      /* Writing a scalar Field on the Quads right here */
      const char fieldname[MED_NAME_SIZE+1] = "TEMPERATURE_FIELD";
      const med_int ncomponent = 1;
      const char componentname[MED_SNAME_SIZE+1] = "TEMPERATURE";
      const char componentunit[MED_SNAME_SIZE+1] = "C";

      if (MEDfieldCr(fid, fieldname, MED_FLOAT64,
                     ncomponent, componentname, componentunit,"",
                     meshname) < 0) {
        MESSAGE("ERROR : create field");
      }

      /* write values at cell (QUADS) centers */
      med_float quad4values[nquad4];
      for (int i=0; i<nquad4; i++)
        quad4values[i] = i%100 + 1;

      med_float quad4values4[nquad4 * 4];
      long int counter = 0;
      for (int i=0; i<nquad4; i++)
        {
        quad4values[i] = i%100 + 1;
        for (int j=0; j<4; j++)
          {
          quad4values4[counter] = quad4values[i];
          counter++;
          }
        }
      if (MEDfieldValueWr(fid, fieldname, MED_NO_DT, MED_NO_IT, 0.0, MED_CELL,
                         MED_QUAD4, MED_FULL_INTERLACE, MED_ALL_CONSTITUENT,
                         nquad4, (unsigned char*) quad4values) < 0) {
        MESSAGE("ERROR : write field values on MED_QUAD4");
      }

      const char fieldname2[MED_NAME_SIZE+1] = "TEMPERATURE_FIELD_PGAUSS";
      if (MEDfieldCr(fid, fieldname2, MED_FLOAT64,
                     ncomponent, componentname, componentunit,"",
                     meshname) < 0) {
        MESSAGE("ERROR : create field");
      }

      if (MEDfieldValueWithProfileWr(
             fid, fieldname2, MED_NO_DT, MED_NO_IT, 0.0, MED_CELL, MED_QUAD4,
             MED_GLOBAL_PFLMODE, MED_NO_PROFILE, localizationName,
           MED_FULL_INTERLACE, MED_ALL_CONSTITUENT,
           nquad4, (unsigned char*) quad4values4) < 0) {
        MESSAGE("ERROR : write field values on MED_QUAD4");
      }

      const char fieldname3[MED_NAME_SIZE+1] = "TEMPERATURE_FIELD_ELNO";
      if (MEDfieldCr(fid, fieldname3, MED_FLOAT64,
                     ncomponent, componentname, componentunit,"",
                     meshname) < 0) {
        MESSAGE("ERROR : create field");
      }

      if (MEDfieldValueWithProfileWr(
             fid, fieldname3, MED_NO_DT, MED_NO_IT, 0.0, MED_CELL, MED_QUAD4,
             MED_GLOBAL_PFLMODE, MED_NO_PROFILE, MED_GAUSS_ELNO,
           MED_FULL_INTERLACE, MED_ALL_CONSTITUENT,
           nquad4, (unsigned char*) quad4values4) < 0) {
        MESSAGE("ERROR : write field values on MED_QUAD4");
      }

      if ( MEDfileClose( fid ) < 0) {
        MED_ERR_(_ret,MED_ERR_CLOSE,MED_ERR_FILE,"");;
      }

      printf("File UsesCase_MEDmesh_parallel.med has been generated.\n");
    } /* End of process ZERO */

  /* MPI_Finalize must be called AFTER MEDclose which may use MPI calls */
  MPI_Finalize();
}
