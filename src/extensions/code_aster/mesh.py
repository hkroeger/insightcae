
import os, sys, csv, re, numpy


class Point:
    def __init__(self, x, y, z):
        self.x=x
        self.y=y
        self.z=z
        
    def __hash__(self):
        return hash((float(self.x), float(self.y), float(self.z)))

class AsterMeshFileObject:
    def __init__(self, key, name):
        self.key=key
        self.name=name
        self.nodes=[]
        
    def write(self, f, nodenames):
        f.write("%s %s\n"%(self.key, self.name))
        f.write(self.content(nodenames))
        f.write("FINSF\n")


class POI1(AsterMeshFileObject):
    def __init__(self, x, group_ma):
        self.x=x
        self.nodes=[x]
        self.group_ma=group_ma
        self.elements=[self.group_ma+"1"]
        
    def write(self, f, nodenames):
        f.write("POI1\n")
        f.write("%s1 %s\n"%(self.group_ma, nodenames[self.x]))
        f.write("FINSF\n")
        f.write("GROUP_MA\n")
        f.write("%s %s1\n"%(self.group_ma, self.group_ma))
        f.write("FINSF\n")

class ElementGroup(AsterMeshFileObject):
    def __init__(self, name, objs):
        self.name=name
        self.elements=[]
        self.nodes=[]
        for o in objs:
            self.elements += o.elements
            
    def write(self, f, nodenames):
        f.write("GROUP_MA\n")
        f.write("%s\n"%(self.name))
        for e in self.elements:
            f.write("%s\n"%(e))            
        f.write("FINSF\n")

class AsterMeshFile:
    
    def __init__(self, unit=20):
        self.unit=unit
        self.objs=[]
        
    def enumerateNodes(self):
        self.nodenames={}
        for o in self.objs:
            for n in o.nodes:
                self.nodenames[n]=1
        for i,nn in enumerate(self.nodenames):
            self.nodenames[nn]="N%d"%(i+1)
        
        
    def write(self):
        
        self.enumerateNodes()
        
        f=open("fort.%d"%self.unit, 'w')
        
        if (len(self.nodenames)>0):
            f.write("COOR_3D\n")
            for co,n in self.nodenames.items():
                f.write("%s %g %g %g\n"%(n, co.x, co.y, co.z))
            f.write("FINSF\n")
        
        for o in self.objs:
            o.write(f, self.nodenames)
            
        f.write("FIN\n")
