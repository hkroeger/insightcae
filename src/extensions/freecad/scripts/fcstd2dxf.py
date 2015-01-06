#!/usr/bin/env python
#/home/hannes/Projekte/freecad-master/build/bin/FreeCADCmd

import os, sys, tempfile
print sys.argv
fn=sys.argv[1]
dn=os.path.splitext(os.path.basename(fn))[0]
sn=sys.argv[2]
on=sys.argv[3]

print fn, ", ", dn, ", ", sn, ", ", on

scr="""
import FreeCAD
import importDXF

FreeCAD.open("%s")
__objs__=[]
doc=FreeCAD.getDocument( "%s" );
print dir(doc)
obj=None #doc.getObject("s")
for o in doc.Objects:
 if (o.Label=="%s"):
  obj=o
  break
print obj
__objs__.append(obj)
importDXF.export(__objs__, "%s")
del __objs__
"""%(os.path.abspath(fn), dn, sn, on)
print scr

f=tempfile.NamedTemporaryFile(suffix=".FCMacro", delete=False)
f.write(scr)
f.close()

print f.name
os.system("FreeCADCmd %s"%f.name)
os.remove(os.path.abspath(f.name))

