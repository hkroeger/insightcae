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
 * TestMedDescendingPolyhedron.cxx
 *
 *  Created on: 3 mars 2011
 *      Author: alejandro
 */

#include <med.h>
#define MESGERR 1
#include <med_utils.h>

#include <string.h>

int main (int argc, char **argv) {
  med_idt fid;
  const char meshname[MED_NAME_SIZE+1] = "3D unstructured mesh";
  const med_int spacedim = 3;
  const med_int meshdim = 3;
  /*                                         12345678901234561234567890123456 */
  const char axisname[3*MED_SNAME_SIZE+1] = "x               y               z               ";
  const char unitname[3*MED_SNAME_SIZE+1] = "cm              cm              cm              ";
  const med_int nnodes = 12;
  const med_float coordinates[3 * 12] =
          { 1.618,  1.,     0.,
           -1.618,  1.,     0.,
            1.618, -1.,     0.,
           -1.618, -1.,     0.,
            1.,     0.,     1.618,
            1.,     0.,    -1.618,
           -1.,     0.,     1.618,
           -1.,     0.,    -1.618,
            0.,  1.618,     1.,
            0., -1.618,     1.,
            0.,  1.618,    -1.,
            0., -1.618,    -1.  };
  const med_int nodeIndexSize = 20;
  const med_int nodeindex[20] = {
          MED_TRIA3, MED_TRIA3, MED_TRIA3, MED_TRIA3, MED_TRIA3,
          MED_TRIA3, MED_TRIA3, MED_TRIA3, MED_TRIA3, MED_TRIA3,
          MED_TRIA3, MED_TRIA3, MED_TRIA3, MED_TRIA3, MED_TRIA3,
          MED_TRIA3, MED_TRIA3, MED_TRIA3, MED_TRIA3, MED_TRIA3 };
  const med_int faceIndexSize = 2;
  /* connectivity : 1 icosahedron */
  const med_int faceindex[2] = { 1, 21 };

  const med_int connectivity[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 9,10,
                                    11,12,13,14,15,16,17,18,19,20 };

  const med_int ntria3 = 20;
  const med_int triaConnectivity[60] = { 1,  9,  5,
                                     1,  6, 11,
                                     3,  5, 10,
                                     3, 12,  6,
                                     2,  7,  9,
                                     2, 11,  8,
                                     4, 10,  7,
                                     4,  8, 12,
                                     1, 11,  9,
                                     2,  9, 11,
                                     3, 10, 12,
                                     4, 10, 12,
                                     5,  3,  1,
                                     6,  1,  3,
                                     7,  2,  4,
                                     8,  4,  2,
                                     9,  7,  5,
                                    10,  5,  7,
                                    11,  6,  8,
                                    12,  8,  6  };

  /* open MED file */
  fid = MEDfileOpen("UsesCase_MEDmesh_17.med",
        MED_ACC_CREAT);
  if (fid < 0) {
    MESSAGE("ERROR : file creation ...");
    return -1;
  }

  /* write a comment in the file */
  if (MEDfileCommentWr(fid,
           "A 3D unstructured mesh : 1 icosahedron") < 0) {
    MESSAGE("ERROR : write file description ...");
    return -1;
  }

  /* mesh creation : a 3D unstructured mesh */
  if (MEDmeshCr(fid,
    meshname,
    spacedim,
    meshdim,
    MED_UNSTRUCTURED_MESH,
    "A 3D mesh with 1 icosahedron in DESCENDING connectivity",
    "",
    MED_SORT_DTIT,
    MED_CARTESIAN,
    axisname,
    unitname) < 0) {
    MESSAGE("ERROR : mesh creation ...");
    return -1;
  }

  /* nodes coordinates in a cartesian axis in full interlace mode
     (X1,Y1, X2,Y2, X3,Y3, ...) with no iteration and computation step
  */
  if (MEDmeshNodeCoordinateWr(fid,
            meshname,
            MED_NO_DT,
            MED_NO_IT,
            MED_UNDEF_DT,
            MED_FULL_INTERLACE,
            nnodes,
            coordinates) < 0) {
    MESSAGE("ERROR : nodes coordinates ...");
    return -1;
  }

  // cells connectiviy is defined in nodal mode
  // it is later in the polyhedron thet the connectivity is descending
  if (MEDmeshElementConnectivityWr(fid,
           meshname,
           MED_NO_DT,
           MED_NO_IT,
           0.0,
           MED_DESCENDING_FACE,
           MED_TRIA3,
           MED_NODAL,
           MED_FULL_INTERLACE,
           ntria3,
           triaConnectivity) < 0) {
    MESSAGE("ERROR : triangular cells connectivity ...");
    return -1;
  }

  // cells connectiviy is defined in descending mode
  // 1 icosahedron
  if (MEDmeshPolyhedronWr(fid,
              meshname,
              MED_NO_DT,
              MED_NO_IT,
              MED_UNDEF_DT,
              MED_CELL,
              MED_DESCENDING,
              faceIndexSize,
              faceindex,
              nodeIndexSize,
              nodeindex,
              connectivity) < 0) {
    MESSAGE("ERROR : polyhedron connectivity ...");
    return -1;
  }

  /* create family 0 : by default, all mesh entities family number is 0 */
  if (MEDfamilyCr(fid,
      meshname,
      "",
      0,
      0,
      "") < 0) {
    MESSAGE("ERROR : quadrangular cells connectivity ...");
    return -1;
  }

  /* close MED file */
  if (MEDfileClose(fid)  < 0) {
    MESSAGE("ERROR : close file ...");
    return -1;
  }

  return 0;
}
