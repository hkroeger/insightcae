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
        [0.033, 0, 0,],
        [-0.153463, 0, 1],
        [-0.398491, 0, 3],
        [-0.435515,1,1],
        [0.664045,2,0],
        [0.283225,2,2],
        [-0.162764,3,0],
        [-0.017208,6,1]
        ]

    B=[
        [-0.000813,0,0],
        [0.034885,0,1],
        [-0.276187,0,3],
        [-0.626198,1,1],
        [0.450379,1,2],
        [0.359718,2,0],
        [-0.087289,3,0],
        [-0.003751,6,2]
        ]
        
    C=[
        [0.00721,0,0],
        [-0.01467,0,2],
        [-0.006398,0,4],
        [-0.03138,1,2],
        [0.010386,2,2],
        [0.053169,3,0],
        [-0.014731,4,0]
        ]
        
    def K(self, J, PbyD, AeByA0, z, coeffs):
        if (abs(AeByA0-0.75)>1e-6):
            raise Exception("Invalid area ratio: "+str(AeByA0)+". For Ka4-70 this is fixed to 0.7!")
        if (z!=5):
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