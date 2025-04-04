#!/usr/bin/env python3

import matplotlib.pyplot as plt
import scipy.optimize as opt
import numpy as np

species="NC12H26"

if species=="C4H10":
        Tlow           =250
        Thigh           =5000
        Tcommon         =1399
        highCpCoeffs    =( 1.39197290e+01, 1.85551492e-02, -6.36014179e-06, 9.88844645e-10, -5.74274779e-14, -2.09452548e+04, -5.06788231e+01 )
        lowCpCoeffs     =( -2.42033073e+00, 5.79308508e-02, -4.30236499e-05, 1.66679793e-08, -2.64714350e-12, -1.53950880e+04, 3.66425242e+01 )
elif species=="C5H12":
        Tlow           =250;
        Thigh           =5000;
        Tcommon         =1389;
        highCpCoeffs    =( 1.96655539e+01, 2.53946113e-02, -8.77477373e-06, 1.37145913e-09, -7.99350015e-14, -2.26521255e+04, -7.38934310e+01 );
        lowCpCoeffs     =( 7.10786402e-01, 7.03521821e-02, -5.11565299e-05, 2.03005197e-08, -3.42470585e-12, -1.59903126e+04, 2.79113132e+01 );
elif species=="C6H14": 
        Tlow            =250;
        Thigh           =5000;
        Tcommon         =1387;
        highCpCoeffs    =( 2.87780675e+01, 2.95251779e-02, -1.02931692e-05, 1.61854521e-09, -9.47401775e-14, -3.87743825e+04, -1.15090906e+02 );
        lowCpCoeffs     =( 2.19542973e+00, 9.38208455e-02, -7.19541247e-05, 2.94849979e-08, -5.04751943e-12, -2.96046234e+04, 2.71617731e+01 );
elif species=="C8H18":
        Tlow            =250;
        Thigh           =5000;
        Tcommon         =1390;
        highCpCoeffs    =( 2.99669394e+01, 3.36833754e-02, -1.12165522e-05, 1.71225938e-09, -9.82418884e-14, -4.15420661e+04, -1.32271564e+02 );
        lowCpCoeffs     =( -2.87689827e+00, 1.11482896e-01, -8.08081607e-05, 2.96268389e-08, -4.33297023e-12, -3.04051599e+04, 4.34716318e+01 );
elif species=="NC10H22":
        Tlow            =300;
        Thigh           =5000;
        Tcommon         =1391;
        highCpCoeffs    =( -2.08416969E+00 ,1.22535012E-01, -7.76815739E-05, 2.49834877E-08, -3.23548038E-12, -3.43021863E+04, 4.42260140E+01 );
        lowCpCoeffs     =( 3.19882239E+01, 4.77244922E-02, -1.62276391E-05, 2.50963259E-09, -1.45215772E-13, -4.66392840E+04, -1.40504121E+02 );
elif species=="NC12H26":
        Tlow            =300;
        Thigh           =5000;
        Tcommon         =1391;
        highCpCoeffs    =( -2.62181594E+00, 1.47237711E-01, -9.43970271E-05, 3.07441268E-08, -4.03602230E-12, -4.00654253E+04, 5.00994626E+01 );
        lowCpCoeffs     =( 3.85095037E+01, 5.63550048E-02, -1.91493200E-05, 2.96024862E-09, -1.71244150E-13, -5.48843465E+04, -1.72670922E+02 );


TcommonNew=1000;

highCpCoeffs=np.array(highCpCoeffs)
lowCpCoeffs=np.array(lowCpCoeffs)


print("highCoeffs=", highCpCoeffs)
print("lowCoeffs=", lowCpCoeffs)

def evalJanafCP(T, a):
	return ((((a[4]*T + a[3])*T + a[2])*T + a[1])*T + a[0])

def evalJanafH(T, a):
	return ((((a[4]/5.0*T + a[3]/4.0)*T + a[2]/3.0)*T + a[1]/2.0)*T + a[0])*T + a[5]

def evalJanafS(T, a):
	return (((a[4]/4.0*T + a[3]/3.0)*T + a[2]/2.0)*T + a[1])*T + a[0]*np.log(T) + a[6]
	
	
	
	
tlow=np.linspace(Tlow, Tcommon, 200)
thigh=np.linspace(Tcommon, Thigh, 200)
tlowNew=np.linspace(Tlow, TcommonNew, 200)
thighNew=np.linspace(TcommonNew, Thigh, 200)

tall=np.linspace(Tlow, Thigh, 400)

def Q(chiclo):
	hcp=chiclo[0:7]
	lcp=chiclo[7:14]
	
	jumpCP=(evalJanafCP(TcommonNew, hcp)-evalJanafCP(TcommonNew, lcp))*100.
	jumpH=(evalJanafH(TcommonNew, hcp)-evalJanafH(TcommonNew, lcp))*1e-4*100.
	jumpS=(evalJanafS(TcommonNew, hcp)-evalJanafS(TcommonNew, lcp))*100.
	
	diffCP = \
	 np.concatenate( (evalJanafCP(tall[tall<=TcommonNew],lcp),evalJanafCP(tall[tall>TcommonNew],hcp)) ) \
	 - \
	 np.concatenate( (evalJanafCP(tall[tall<=Tcommon],lowCpCoeffs),evalJanafCP(tall[tall>Tcommon],highCpCoeffs)) )

	diffH = (\
	 np.concatenate( (evalJanafH(tall[tall<=TcommonNew],lcp),evalJanafH(tall[tall>TcommonNew],hcp)) ) \
	 - \
	 np.concatenate( (evalJanafH(tall[tall<=Tcommon],lowCpCoeffs),evalJanafH(tall[tall>Tcommon],highCpCoeffs)) ) )*1e-4

	diffS = \
	 np.concatenate( (evalJanafS(tall[tall<=TcommonNew],lcp),evalJanafS(tall[tall>TcommonNew],hcp)) ) \
	 - \
	 np.concatenate( (evalJanafS(tall[tall<=Tcommon],lowCpCoeffs),evalJanafS(tall[tall>Tcommon],highCpCoeffs)) )
	 
	 
	#print(np.concatenate( (evalJanafCP(tlon,lcp),evalJanafCP(thin,hcp))) )
	#print(jump)
	q = np.sum(diffCP**2) + np.sum(diffH**2) + np.sum(diffS**2) + jumpCP**2 + jumpH**2 + jumpS**2
	print ("q=", q)
	return q

a0=np.concatenate((highCpCoeffs, lowCpCoeffs))

print("begin opt")
ores=opt.minimize(Q, a0, method='Nelder-Mead', tol=1e-10, options={'maxiter':1000000})

print(ores)
highCpCoeffsNew=ores.x[0:7]
lowCpCoeffsNew=ores.x[7:14]

print("new highCoeffs=", highCpCoeffsNew)
print("new lowCoeffs=", lowCpCoeffsNew)



print ("Jump CP old = ", evalJanafCP(Tcommon, highCpCoeffs)-evalJanafCP(Tcommon, lowCpCoeffs))
print ("Jump H old = ", evalJanafH(Tcommon, highCpCoeffs)-evalJanafH(Tcommon, lowCpCoeffs))
print ("Jump S old = ", evalJanafS(Tcommon, highCpCoeffs)-evalJanafS(Tcommon, lowCpCoeffs))

print ("Jump CP new = ", evalJanafCP(TcommonNew, highCpCoeffsNew)-evalJanafCP(TcommonNew, lowCpCoeffsNew))
print ("Jump H new = ", evalJanafH(TcommonNew, highCpCoeffsNew)-evalJanafH(TcommonNew, lowCpCoeffsNew))
print ("Jump S new = ", evalJanafS(TcommonNew, highCpCoeffsNew)-evalJanafS(TcommonNew, lowCpCoeffsNew))




plt.figure()
plt.title('CP')
plt.plot(tlow, evalJanafCP(tlow, lowCpCoeffs), 'r-')
plt.plot(thigh, evalJanafCP(thigh, highCpCoeffs), 'r--')
plt.plot(tlowNew, evalJanafCP(tlowNew, lowCpCoeffsNew), 'g-')
plt.plot(thighNew, evalJanafCP(thighNew, highCpCoeffsNew), 'g--')


plt.figure()
plt.title('Enthalpy')
plt.plot(tlow, evalJanafH(tlow, lowCpCoeffs), 'r-')
plt.plot(thigh, evalJanafH(thigh, highCpCoeffs), 'r--')
plt.plot(tlowNew, evalJanafH(tlowNew, lowCpCoeffsNew), 'g-')
plt.plot(thighNew, evalJanafH(thighNew, highCpCoeffsNew), 'g--')


plt.figure()
plt.title('Entropy')
plt.plot(tlow, evalJanafS(tlow, lowCpCoeffs), 'r-')
plt.plot(thigh, evalJanafS(thigh, highCpCoeffs), 'r--')
plt.plot(tlowNew, evalJanafS(tlowNew, lowCpCoeffsNew), 'g-')
plt.plot(thighNew, evalJanafS(thighNew, highCpCoeffsNew), 'g--')

plt.show()
