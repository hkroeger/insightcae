*******************************************************************************
*******************************************************************************
**********                                                          ***********
**********                                                          ***********
**********    Project for Organized Research Combination System     ***********
**********     	        "Smart Control of Turbulence:               ***********
**********           A Millennium Challenge for Innovative          ***********
**********                Thermal and Fluids Systems"               ***********
**********                                                          ***********
**********                                                          ***********
**********          Supported by the Ministry of Education,         ***********
**********           Culture, Science and Technology (MEXT)         ***********
**********                                                          ***********
**********                       2000 - 2004                        ***********
**********                                                          ***********
**********                                                          ***********
**********                        N. Kasagi                         ***********
**********    Director of Center for Smart Control of Turbulence    ***********
**********                                                          ***********
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
Test case: Fully Developed Turbulent Pipe Flow

Code number: PI12__PG.WL3

Date of Release: Septermber 21, 2002

Computors:     K. Fukagata and N. Kasagi
               Department of Mechanical Engineering
               The University of Tokyo
               Bunkyo-ku, Tokyo 113-8656

Nomenclature: See headers of each data set.


1. Description of Flow Field

The flow field simulated is a fully developed turbulent flow in a cylindrical 
pipe. The flow is homogeneous both in the longitudinal and azimuthal directions
and the statistics are dependent only upon the distance from the wall.  The 
data presented here are non-dimensionalized by the wall variables, i.e., u_tau
and nu.  The flow condition is defined by the pressure gradient imposed and the
pipe radius.


2. Numerical Method

Governing equations:  Incompressible Navier-Stokes equations with primitive 
variables, i.e., u_r (radial velocity), u_t (azimuthal velocity), u_z 
(longitudinal velocity), p (pressure), and the continuity equation.

Discretization method: Highly energy-conservative second-order finite 
difference method (Ref. [1]) with a staggered grid system.

Removal of singularity at the cylindrical axis: a mathematically consistent 
method of interpolation (Ref. [1]).

Size of computational domain:  R x 2 \pi R x 10 R

Number of grid: 96 x 128 x 256

Grid spacing:  0.46 - 2.99 viscous units in the r-direction, 8.84 - 0.074 and 
7.03 in the theta- and z- directions.

Time integration: third order Runge-Kutta scheme for the nonlinear terms and 
Crank-Nicolson scheme for the viscous terms.

Initial conditions: logarithmic mean + random fluctuations; developed under 
reduced viscosity (similarly to Eggels et al. (1994)).

Criterion for stationary state:  
(1) linear profile of total stress; 
(2) stationary behavior of mean velocity and zero azimuthal mean velocity;
(3) zero skewness factor of azimuthal velocity; 
(4) zero sum. of Reynolds stress budget.

Length of time integration for ensemble averaging: 2160 wall unit time after 
the fully developed state is reached.

Error in continuity equation = Max [abs {div(v)} ] : about 3e-16 wall unit.

Courant number = Max [ delta-t * {abs(u_i/delta-x_i)} ] :  C < 0.1 (near the 
wall); C < 5 (at the first points from the cylindrical axis, in azimuthal 
direction)

Computer: HITACHI SR8000/128 at Computer Centre of the University of Tokyo.

Computation time:  about 2.2 sec. (with 1 PE = 8 CPUs) per one time step (= 
three sub-time step).


3. Flow Conditions

     Re_tau = R*u_tau/nu = 180
     Re_m = 2*R*Um/nu = 5310

Budget of Reynolds stress and turbulent kinetic energy

# Nomenclature
# 	y+	Distance from the wall in wall unit 
#	P+	Production
#	TD+	Turbulent diffusion
#	TCR+	Turbulent-centrifugal redistribution
#(Turbulent diffusion in conventional decomposition is TD + TCR, see Ref. 5.2)
#	PD+	Pressure diffusion
#	PCR+	Pressure-centrifugal redistribution
#(Pressure diffusion in conventional decomposition is PD + PCR, see Ref. 5.2)
#	PS+	Pressure-strain
#	VD+	Viscous diffusion
#	VCR+	Viscous-centrifugal redistribution
#(Viscous diffusion in conventional decomposition is VD + VCR, see Ref. 5.2)
#	D+	Dissipation
#	balance	Summation of all the terms
#
#	Note: The terms which are theoretically zero were not computed 
#	      and indicated below, e.g. if the production term is such,
#	      as, "P+(NC)".
#