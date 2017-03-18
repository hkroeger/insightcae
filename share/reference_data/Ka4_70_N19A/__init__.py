# -*- coding: utf-8 -*-

import math, re
import numpy as np
import scipy.interpolate as interp
import scipy.optimize as opt
from .. import propellerTools as pt

class OpenWaterCurve(object):

    PDmin=0.6
    PDmax=1.4

    A=[
        [0.030550, 0, 0,],
        [-0.148687, 0, 1],
        [-0.391137, 0, 3],
        [-0.432612, 1, 1],
        [0.667657, 2, 0], 
        [0.285076, 2, 2],
        [-0.172529, 3, 0],
        [-0.017283, 6, 1]
        ]
        
    B=[
        [0.076594,0,0],
        [0.075223,0,1],
        [-0.061881,0,2],
        [-0.138094,0,3],
        [-0.37062,0,5],
        [0.323447,0,6],
        [-0.271337,1,0],
        [-0.687921,1,1],
        [0.225189,1,2],
        [-0.081101,1,6],
        [0.666028,2,0],
        [0.734285,2,2],
        [-0.202467,3,0],
        [-0.54249,3,2],
        [-0.016149,3,6],
        [0.099819,4,3],
        [0.030084,5,1],
        [-0.001876,6,2]
        ] 
    
    C=[
        [0.006735, 0, 0],
        [-0.016306, 0, 2],
        [-0.007244, 0, 4],
        [-0.024012, 1, 2],
        [0.005193, 2, 2],
        [0.046605, 3, 0],
        [-0.007366, 4, 0],
        [-0.00173, 6, 0],
        [-0.000337, 6, 1],
        [0.000861, 6, 2]
        ]
        
    def K(self, J, PbyD, AeByA0, z, coeffs):
        if (abs(AeByA0-0.7)>1e-6):
            raise Exception("Invalid area ratio: "+str(AeByA0)+". For Ka4-70 this is fixed to 0.7!")
        if (z!=4):
            raise Exception("Invalid blade number: "+str(z)+". For Ka4-70 this is fixed to 4!")
        K = 0.0
        for c in coeffs:
            K = K + \
                c[0] * (J ** c[2]) * (PbyD ** c[1])
        return K

    def Kt(self, J, PbyD, AeByA0, z):
        return self.K(J, PbyD, AeByA0, z, self.A)

    def Ktn(self, J, PbyD, AeByA0, z):
        return self.K(J, PbyD, AeByA0, z, self.B)

    def Kq(self, J, PbyD, AeByA0, z):
        return self.K(J, PbyD, AeByA0, z, self.C)
    
def getToC():
  return pt.getToC()


def getProfile(name):
  return pt.getProfile(OpenWaterCurve(), name)