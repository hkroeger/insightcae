*******************************************************************************
*******************************************************************************
**********                                                          ***********
**********                        <Group #2>                        ***********
**********    "Modeling Basic Transport Processes of Turbulence"    ***********
**********                                                          ***********
**********                                                          ***********
**********        "Mathematical Modeling of Turbulent Flows"        ***********
**********                                                          ***********
**********           Scientific Research on Priority Areas          ***********
**********                       No. 05240103                       ***********
**********                                                          ***********
**********           Supported by the Ministry of Education,        ***********
**********                    Science and Culture                   ***********
**********                                                          ***********
**********                       1993 - 1996                        ***********
**********                                                          ***********
**********                                                          ***********
**********                      N. Kasagi (PI)                      ***********
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
**********  of Dr. J.G.M. Eggels.                                    ***********
**********                                                          ***********
*******************************************************************************

*******************************************************************************
<Author's Note>
FILE CAN BE DISTRIBUTED FREELY TO OTHERS WHO ARE INTERESTED, BUT WE
APPRECIATE TO RECIEVE A MESSAGE (E-MAIL OR FAX) IN CASE SOMEONE RECIEVES
THESE DATA NOT DIRECTLY FROM US. PLEASE DON'T FORGET TO REFER TO ONE OF THE
PUBLICATIONS MENTIONED BELOW IN CASE YOU USE THESE DATA IN PUBLICATION(S). 

DELFT, THE NETHERLANDS  JACK EGGELS     NOVEMBER 12, 1993
*******************************************************************************

Test Case: Fully Developed Turbulent Pipe Flow

Code Number: PI12__PG.WL2

Date of Release: July 19, 1995

Date of Revision:

Computor:      dr.ir. J.G.M. Eggels (Jack)
               Shell Research B.V.
               Koninklijke/Shell-Laboratorium, Amsterdam (KSLA)
               Department Measurement and Computational Applications (MCA/2)
               P.O. Box 38000
               1030 BN Amsterdam
               The Netherlands

               tel : +31.20.6303246
               fax : +31.20.6304041
               e-mail : eggels1@ksla.nl

Nomenclature:

      N_r, N_theta, N_z : the number of gridpoints in all three directions.
      Re_b, Re_c, Re_* : the Reynolds numbers based on pipe diameter and bulk 
               velocity, centerline velocity and wall shear stress velocity 
               respectively.
      dr, R dtheta, dz : grid spacing in viscous units in radial, tangential 
               (at the pipe wall) and axial direction respectively.


1. Description of Flow Field

This datafile contains various statistical results obtained from Direct
Numerical Simulation (DNS) of fully developed turbulent pipe flow at
low-Reynolds number. The characteristics of the simulation are as follows:

Domain size     :       D       2 pi    5 D
N_r, N_theta, N_z : 96  128     256
Re_b, Re_c, Re_* : 5300 7000    360
dr, R dtheta, dz :      1.88    8.84    7.03

2. Numerical Method

The details of the numerical techniques, of the simulation itself and of
the comparison of numerical results with experimental data can be found in
the publications listed at the end of this database. In case the data in
this file are used in publications, please refer to one (or more) of the
references.

Notes:

a) The simulation is performed using a staggered grid which means 
that not all statistics are available at the same radial location. 
b) To convert the position r/D into y+, the following relation should 
be used: y+ = ( 0.5 - r/D ) * Re_*
c) The radial velocity component is defined POSITIVE from the centerline 
towards the pipe wall (see e.g. Reynolds shear stress). 
d) Unless mentioned explicitly, all results are scaled using the wall 
shear stress velocity u_* and the pipe diameter D as the velocity and length 
scale for the normalization.

3. Numerical Data

3.1 Mean Flow Quantities

Mean (bulk) velocity : U_b = 14.73 u_*
Centerline velocity : U_c = 19.31 u_*
Friction coefficient : C_f = 9.22E-3
Friction coefficient : C_f = 9.27E-3 (based on Blasius' law) 

Energy budgets of the Reynolds stresses

For the four non-zero Reynolds stresses, the various contributions in the
energy budgets of these stresses are listed below. A positive contribution
denotes gain of stress whereas a negative value indicates loss. The sum of
all terms at each radial location is reported in the last column and
represents the unbalance in the energy budget. The following contributions
are computed: 

TD : turbulent diffusion
PR : production
VP : velocity pressure-gradient interaction VD : viscous diffusion
DS : viscous dissipation
SUM : TD + PR + VP + VD + DS (unbalance in budget) 

The governing equations are reported in appendix C of reference 4.3.