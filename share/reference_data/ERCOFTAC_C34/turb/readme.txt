DATA BASE SHL04 
directory: turb

___________________________________________________________________________
list of included files 

read.me	: this file
0150.m1
0200.m1
0250.m1
0650.m1
0800.m1
0950.m1
0150.m2
0200.m2
0250.m2
0650.m2
0800.m2
0950.m2
0150.m3
0200.m3
0250.m3
0650.m3
0800.m3
0950.m3
eps.200
eps.800
kuv.200
kuv.800

___________________________________________________________________________
Files descriptions:

files DDDD.mX : evolution of moments of order X measured at DDDD mm from 
		trailing edge of splitting plate.

files eps.DDD : evolution of dissipation measured at DDD mm from trailing
		of splitting plate.

files kuv.DDD : evolution of Reynolds stress <uv> and turbulent kinetic energy
		measured around DDD mm from trailing of  splitting plate.


notations: < > corresponds to temporal average
	   u, v, w are fluctuating velocity components (resp. longitudinal,
	           normal to the plate, spanwise)
           delta_omega is the vorticity thickness
	   Delta_U is the difference of velocity (high speed - low speed)
	
_____________________________________________________________________________
files DDDD.m1 : evolution of 1st order moments (i.e. mean value)   of u and v.
                stored are:
		col. #1  y/delta_omega  
		col. #2  <u>/Delta_U
		col. #3  <v>/Delta_U
note: <w> = 0 from homogeneity in the spanwise direction

_____________________________________________________________________________
files DDDD.m2 : evolution of 2nd order moments  of u, v and w.
                stored are:
		col. #1  y/delta_omega  
		col. #2  <u*u>/Delta_U**2 
		col. #3  <v*v>/Delta_U**2 
		col. #4  <u*v>/Delta_U**2 
		col. #5  <w*w>/Delta_U**2 
note: <u*w>  = <v*w> = 0 from homogeneity in the spanwise direction

_____________________________________________________________________________
files DDDD.m3 : evolution of 3rd order moments  of u, v and w.
                stored are:
		col. #1  y/delta_omega  
		col. #2  <u*u*u>/Delta_U**3 
		col. #3  <v*v*v>/Delta_U**3 
		col. #4  <u*u*v>/Delta_U**3 
		col. #5  <u*v*v>/Delta_U**3 
		col. #6  <u*w*w>/Delta_U**3 

extra col. only for DDDD = 0200 and DDDD = 0800:
		col. #7  <v*w*w>/Delta_U**3 

note: <u*u*w> = <v*v*w> = <u*v*w> = <w*w*w> = 0  from homogeneity in the
      spanwise  direction

_____________________________________________________________________________


files eps.DDD : evolution of dissipation epsilon 
                stored are:
		col. #1  y/delta_omega  
		col. #2  epsilon*Delta_U**2/delta_omega

_____________________________________________________________________________

file kuv.200 :evolution of turbulent kinetic energu and Reynolds stress <uv>
	      in the region 200 mm from trailing edge of splitting plate

		col. #1 y/delta_omega   at x=150
		col. #2 k/Delta_U**2    at x=150
		col. #3 <uv>/Delta_U**2 at x=150

		col. #4 y/delta_omega   at x=200
		col. #5 k/Delta_U**2    at x=200
		col. #6 <uv>/Delta_U**2 at x=200

		col. #7 y/delta_omega   at x=250
		col. #8 k/Delta_U**2    at x=250
		col. #9 <uv>/Delta_U**2 at x=250

file kuv.800 :evolution of turbulent kinetic energu and Reynolds stress <uv>
	      in the region 800 mm from trailing edge of splitting plate

		col. #1 y/delta_omega   at x=650
		col. #2 k/Delta_U**2    at x=650
		col. #3 <uv>/Delta_U**2 at x=650

		col. #4 y/delta_omega   at x=800
		col. #5 k/Delta_U**2    at x=800
		col. #6 <uv>/Delta_U**2 at x=800

		col. #7 y/delta_omega   at x=950
		col. #8 k/Delta_U**2    at x=950
		col. #9 <uv>/Delta_U**2 at x=950


