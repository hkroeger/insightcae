DATA BASE SHL04 
directory: balance
 
___________________________________________________________________________
list of included files 
 
read.me         : this file

balancek.200
balancuv.200
convk.200
derivl.200
derivs.200
diffk.200
epsl.200
momentum.200
prodk.200

balancek.800
balancuv.800
convk.800
derivl.800
derivs.800
diffk.800
epsl.800
momentum.800
prodk.800

___________________________________________________________________________
Files descriptions:


These files contains data to balance:
energy,   shear stress and momentum balance   equations.This quantities are
normalized by the velocity difference Delta_U and the vorticity thickness
delta_omega. The balances are established at two streamwise locations:

200mm from trailing edge :    files *.200
800mm from trailing edge :    files *.800


energy balance is written :
CONVECTION+DIFFUSION+PRODUCTION+DISSIPATION=0

shear-stress balance is written
CONVECTION+DIFFUSION+PRODUCTION+PRESSURE STRAIN=0

notations: < > corresponds to temporal average
           u, v, w are fluctuating velocity components (resp. longitudinal,
                   normal to the plate, spanwise)
           delta_omega is the vorticity thickness
           Delta_U is the difference of velocity (high speed - low speed)
 


___________________________________________________________________________

Files convk.DDD
Terms appearing in  convectio of k
		col. #1 y/delta_omega
		col. #2 U* dk/dx
		col. #3 V* dk/dy
		col. #4 convetion of k


___________________________________________________________________________

Filesdiffk.DDD 
Terms appearing in diffusion  of k
		col. #1  y/delta_omega
		col. #2  -.5*d/dx (<u*u*u + u*v*v + u*w*w>)
		col. #3  -.5*d/dy (<u*u*v + v*v*v + v*w*w>)
		col. #4  diffusion of k

___________________________________________________________________________

Files prodk.DDD
Terms appearing in production of k
		col. #1  y/delta_omega
		col. #2	 -(<u*u - v*v>) * dU/dx 
		col. #3  -<u*v> * dU/dy   
		col. #4   production of k

___________________________________________________________________________

File epsl.DDD
		col. #1  y/delta_omega
		col. #2  dissipation epsilon

___________________________________________________________________________

Files balancek.DDD
Terms appearing in the balance of k
		col. #1  y/delta_omega
		col. #2  remainder of balance
		col. #3  dissipation by difference
		col. #4  dissipation measured

___________________________________________________________________________


Files balancuv.DDD
Terms appearing in the balance of shear stress
		col. #1  y/delta_omega
		col. #2  U* d/dx <u*v>
		col. #3  V* d/dy <u*v>
		col. #4  convection
		col. #5  diffusion 
		col. #6  production
		col. #7  press_strain

___________________________________________________________________________


Files momentum.DDD
Terms appearing in the momentum equation
		col. #1  y/delta_omega
		col. #2  U*dU/dx
		col. #3  V*dU/dy
		col. #4  -d/dy <u*v>
		col. #5  -d/dx(<u*u - v*v>)
		col. #6   U*dU/dx +V*dU/dy
		col. #7  -d/dy <u*v> -d/dx (<u*u - v*v>)
		col. #8   should_be_0
		col. #9  V from momentum balance


