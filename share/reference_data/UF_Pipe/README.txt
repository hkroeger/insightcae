*******************************************************************************
*******************************************************************************
**********                                                          ***********
**********                    "Establishment of                     ***********
**********      the Direct Numerical Simulation Data Bases of       ***********
**********            Turbulent Transport Phenomena"                ***********
**********                                                          ***********
**********                   Co-operative Research                  ***********
**********                       No. 02302043                       ***********
**********                                                          ***********
**********           Supported by the Ministry of Education,        ***********
**********                    Science and Culture                   ***********
**********                                                          ***********
**********                       1990 - 1992                        ***********
**********                                                          ***********
**********                                                          ***********
**********                 <Research Collaborators>                 ***********
**********                                                          ***********
**********                      N. Kasagi (PI)                      ***********
**********       K. Horiuti, Y. Miyake, T. Miyauchi, Y. Nagano      ***********
**********                                                          ***********
*******************************************************************************
*******************************************************************************

*******************************************************************************
*******************************************************************************
**********                       << CAUTION >>                      ***********
**********                                                          ***********
**********  All rights are reserved by the computors of each data   ***********
**********  base.  No part of the data described herein may be      ***********
**********  represented or otherwise used in any form without fully ***********
**********  referring to this data base and the literature cited at ***********
**********  the end of the data base.  The original data base will  ***********
**********  be revised without notice, whenever necessary.          ***********
**********                                                          ***********
*******************************************************************************
*******************************************************************************

=================== this data base begins from this line ======================

*******************************************************************************
**********                                                          ***********
**********  This paticular data set has been provided by courtesy   ***********
**********  of Dr. F. Unger.                                        ***********
**********                                                          ***********
*******************************************************************************

Test Case: Fully Developed Turbulent Pipe Flow

Code Number: PI12__PG.WL1

Date of Release: September 16, 1993

Computors:    F. Unger and R. Friedrich
              Lehrstuhl fuer Fluidmechanik
              Technische Universitaet Muenchen
              Arcisstrasse 21, 80290 Muenchen, Germany
              Tel. +49 89 2105 2506
              Fax  +49 89 2105 2505
              e-mail: fu@lsm.mw.tu-muenchen.d400.de

===========================================================================

NOMENCLATURE:

      D            pipe diameter
      u_tau        friction velocity
      nu           viscosity
      Re           Reynolds number  Re = u_tau*D/nu

COORDINATE SYSTEM:

      cylindrical coordinate system:     z   axial direction
                                         phi circumferential direction
                                         r   radial direction

FLOW PARAMETERS:

      pipe length          5D
      grid resolution:
            z   direction  256 Gridpoints  aequidistant -> Delta_z=5D/256
            phi direction  128 Gridpoints  aequidistant -> Delta_phi=2Pi/128
            r   direction   96 Gridpoints  aequidistant -> Delta_r=0.5D/96

COMMENT:

      Please note that staggered grids are used in the code. The first
      values for u_z and u_phi are defined at Delta_r/2. The corresponding
      radial velocity component u_r is defined at Delta_r.


VALUES FOR NON-DIMENSIONALIZATION:

      D , u_tau   -->  Re = u_tau*D/nu = 360

TIMESTEP AND PROBLEM TIME:

      timestep     Delta_t = 0.0002
      problem time       T = 16
      --> 80000 timesteps

STATISTICAL AVERAGING

      41 ensemble at t=12.0,12.1,12.2,....,15.9,16.0
===========================================================================
