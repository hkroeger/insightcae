AGARD Working Group data base - October 1997
CASE - PCH00
All header information for the data files is contained in this README file.
No headers are included with the individual files.

This README file contains the necessary information to navigate through
the different data files from the pipe flow simulation.  For reference,
the reader is well advised to consult:

	Direct Numerical Simulation of Incompressible Pipe Flow
	Using a B-spline Spectral Method,
	P. Loulou, R.D. Moser, N.N. Mansour, B.J. Cantwell,
	NASA Technical Memorandum - 110436,
	February 1997


1- Non-dimensionalization

All the data included in the files PipeData_#, are non dimensionalized with
respect to:

	R_2, the outer radius		(R_2=1)
	U_b, the bulk velocity		(U_b=1)

Another important dimension is the pipe length:

	L_z = 10,	or 5 diameters

The Reynolds number for the simulation is:

	Re_b = (R_2 U_b) / (\nu) = 2800

Other relevant parameters are (see p. 34):

	Re_c = (D U_c) / (\nu) = 7248, where U_c is the centerline velocity
	Re_\tau = (D u_\tau) / (\nu) = 380, where u_\tau is the friction vel.
	U_c/u_\tau = 19.11
	U_b/u_\tau = 14.77
	U_c/U_b    = 1.29
	C_f        = 0.00916, where C_f is the friction coefficient


2- Nomenclature

The following notation is used to identify the data:

r:		radial coordinate
\theta:		azimuthal coordinate
z:		axial (streamwise) coordinate
m:		non-dimensional streamwise wave number (m = k_z L_z/2\pi)
		(integer 0,1,2,...)
k_\theta:	azimuthal wave number (integer 0,1,2,...)
\delta z/D:	non-dimensional streamwise coordinate (used for 2-point corr.)
r\delta\theta:	non-dimensional circumference (used for 2-point corr.)
U_1:		mean velocity - z coordinate
u_1:		velocity fluctuation - r coordinate
u_2:		velocity fluctuation - \theta coordinate
u_3:		velocity fluctuation - z coordinate
u_1 u_3:	Reynolds shear stress
p:		pressure fluctuation
\Omega_2:	mean vorticity - \theta coordinate
\omega_1:	vorticity fluctuation - r coordinate
\omega_2:	vorticity fluctuation - \theta coordinate
\omega_3:	vorticity fluctuation - z coordinate
RMS:		Root mean square
S(u_i):		skewness - u_i velocity fluctuation
F(u_i):		flatness - u_i velocity fluctuation
P_k:		Production of turbulent kinetic energy
T_k:		Transport  of    "         "      "
\Pi_k:		Pressure diffusion
\D_k:		Viscous diffusion
\epsilon:	Dissipation
P^1_\epsilon:	Mixed production
P^2_\epsilon:	Production by mean velocity gradient
P^3_\epsilon:	Gradient production
P^4_\epsilon:	Turbulent production
T_\epsilon:	turbulent transport
\Pi_\epsilon:	Pressure transport
D_\epsilon:	Viscous diffusion
P_{ij}:		Production - ij component
\epsilon_{ij}:	Dissipation - ij component
T_{ij}:		Turbulent transport - ij component
D_{ij}:		Viscous diffusion - ij component
\Pi_{ij}:	Velocity pressure-gradient - ij component
E(r)_{ij}:	Energy spectra - ij component, r location
\Omega(r)_{ij}:	Vorticity spectra - ij component, r location
Q(r;u)_{ij}:	Two-point velocity correlation - ij component, r location
Q(r;\omega)_{ij}:  Two-point vorticity correlation - ij component, r location

3- Data Files

Here is a list of the files containing the relevant pipe flow simulation
results.

FILENAME;			DATA

PCH00_01;	r, U_1, u_1 u_3, (u_1)RMS, (u_2)RMS, (u_3)RMS, (p)RMS, \Omega_2,
		(\omega_1)RMS, (\omega_2)RMS, (\omega_3)RMS

PCH00_02;	r, S(u_1), S(u_2), S(u_3), F(u_1), F(u_2), F(u_3)

PCH00_03;	r, P_k, T_k, \Pi_k, D_k, \epsilon

PCH00_04;	r, P^1_\epsilon, P^2_\epsilon, P^3_\epsilon, P^4_\epsilon, T_\epsilon,
		\Pi_\epsilon, D_\epsilon

PCH00_05;	r, P_{11), \epsilon_{11}, T_{11}, D_{11}, \Pi_{11}

PCH00_06;	r, P_{22), \epsilon_{22}, T_{22}, D_{22}, \Pi_{22}

PCH00_07;	r, P_{33), \epsilon_{33}, T_{33}, D_{33}, \Pi_{33}

PCH00_08;	r, P_{13), \epsilon_{13}, T_{13}, D_{13}, \Pi_{13}

The next series of files contains energy and vorticity spectra. Four
positions are given; defining the positions as r_i, i=1,2,3,4:
r_1 = y^+ = 169.1 (r/D = 0.055), r_2 = y^+=122 (r/D = 0.1789),
r_3 = y^+=21.99 (r/D = 0.4421) and r_4 = y^+=3.458 (r/D = 0.4909).

PCH0_09;	m, E(r_1)_{11}, E(r_1)_{22}, E(r_2)_{33},
		E(r_2)_{11}, E(r_2)_{22}, E(r_2)_{33},
		E(r_3)_{11}, E(r_3)_{22}, E(r_3)_{33},
		E(r_4)_{11}, E(r_4)_{22}, E(r_4)_{33}

PCH00_10;	k_\theta, E(r_1)_{11}, E(r_1)_{22}, E(r_2)_{33},
		E(r_2)_{11}, E(r_2)_{22}, E(r_2)_{33},
		E(r_3)_{11}, E(r_3)_{22}, E(r_3)_{33},
		E(r_4)_{11}, E(r_4)_{22}, E(r_4)_{33}

PCH00_11;	m, \Omega(r_1)_{11}, \Omega(r_1)_{22}, \Omega(r_2)_{33},
		\Omega(r_2)_{11}, \Omega(r_2)_{22}, \Omega(r_2)_{33},
		\Omega(r_3)_{11}, \Omega(r_3)_{22}, \Omega(r_3)_{33},
		\Omega(r_4)_{11}, \Omega(r_4)_{22}, \Omega(r_4)_{33}

PCH00_12;	k_\theta, \Omega(r_1)_{11}, \Omega(r_1)_{22}, \Omega(r_2)_{33},
		\Omega(r_2)_{11}, \Omega(r_2)_{22}, \Omega(r_2)_{33},
		\Omega(r_3)_{11}, \Omega(r_3)_{22}, \Omega(r_3)_{33},
		\Omega(r_4)_{11}, \Omega(r_4)_{22}, \Omega(r_4)_{33}

These files contain the information to reproduce figures 3.4 to 3.7 in
Loulou et. al. and do not need any normalization.  The next two files contain
the two-point correlation for the streamwise direction:

PCH00_13;	\delta z/D, Q(r_1;u)_{11}, Q(r_1;u)_{22}, Q(r_2;u)_{33},
		Q(r_2;u)_{11}, Q(r_2;u)_{22}, Q(r_2;u)_{33},
		Q(r_3;u)_{11}, Q(r_3;u)_{22}, Q(r_3;u)_{33},
		Q(r_4;u)_{11}, Q(r_4;u)_{22}, Q(r_4;u)_{33}

PCH00_14;	\delta z/D, Q(r_1;\omega)_{11}, Q(r_1;\omega)_{22}, Q(r_2;\omega)_{33},
		Q(r_2;\omega)_{11}, Q(r_2;\omega)_{22}, Q(r_2;\omega)_{33},
		Q(r_3;\omega)_{11}, Q(r_3;\omega)_{22}, Q(r_3;\omega)_{33},
		Q(r_4;\omega)_{11}, Q(r_4;\omega)_{22}, Q(r_4;\omega)_{33}

The last four files give the two-point correlation for azimuthal separation
for only the two positions closest to the wall:

PCH00_15;	r\delta\theta, Q(r_3;u)_{11}, Q(r_3;u)_{22}, Q(r_3;u)_{33}

PCH00_16;	r\delta\theta, Q(r_4;u)_{11}, Q(r_4;u)_{22}, Q(r_4;u)_{33}

PCH00_17;	r\delta\theta, Q(r_3;\omega)_{11}, Q(r_3;\omega)_{22}, Q(r_3;\omega)_{33}

PCH00_18;	r\delta\theta, Q(r_4;\omega)_{11}, Q(r_4;\omega)_{22}, Q(r_4;\omega)_{33}

Again, the two-point correlations do not need any normalization as they are given.

4- Normalizing according to Loulou et. al. (NASA-TM 110436)

In order to recover the figures as per Loulou et. al.
use the following parameters to normalize to wall units
(or to a more customary form) the data included in PCH00_#:

QUANTITY				NORMALIZATION PARAMETER (divide by...)

(u_i)RMS				u_\tau
\bar{u_1 u_2}				{u_\tau}^2
(p)RMS					{u_\tau}^2
(\omega_i)RMS				{u_\tau}^2 Re_b
S(u_i(r))				{(u_i(r))RMS}^3
F(u_i(r))				{(u_i(r))RMS}^4
P_k, T_k, \Pi_k, D_k, \epsilon,
P_{ij}, \epsilon_{ij}, T_{ij},
D_{ij}, \Pi_{ij}			{u_\tau}^4 Re_b
P^1_\epsilon, P^2_\epsilon,
P^3_\epsilon, P^4_\epsilon,
T_\epsilon, \Pi_\epsilon, D_\epsilon	{u_\tau}^6 {Re_b}^3
