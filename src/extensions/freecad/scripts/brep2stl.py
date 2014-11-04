#!/usr/bin/python

import os, sys

from numpy import *
from numpy.linalg import *

os.environ['LANG']='C'

from OCC.Display.SimpleGui import *

from OCC.Standard import *
from OCC.TopoDS import *
from OCC.TopAbs import *
from OCC.TopExp import *
from OCC.TopTools import *
from OCC.TColgp import *
from OCC.ShapeFix import *

from OCC.IGESControl import *
from OCC.STEPControl import *

from OCC.BRep import *
from OCC.BRepMesh import *
from OCC.BRepLib import *
from OCC.BRepFill import *
from OCC.BRepAdaptor import *
from OCC.BRepTools import *
from OCC.BRepOffsetAPI import *
from OCC.BRepPrimAPI import *
from OCC.BRepAlgoAPI import *
from OCC.BRepBuilderAPI import *
from OCC.BRepGProp import *

from OCC.gp import *
from OCC.GCE2d import *
from OCC.GC import *
from OCC.GProp import *

from OCC.Geom import *
from OCC.Geom2d import *
from OCC.Geom2dAPI import *
from OCC.GeomAbs import *
from OCC.GeomAPI import *
from OCC.GeomConvert import *
from OCC.GeomPlate import *
from OCC.GeomLProp import *

from OCC.BOPTools import *
from OCC.IntTools import *
from OCC.Approx import *

from OCC.GeomAdaptor import *
from OCC.Adaptor3d import *
from OCC.Adaptor2d import *
from OCC.Convert import *
from OCC.StlAPI import *

from OCC.ShapeAnalysis import *

shape=TopoDS_Shape()

builder=BRep_Builder()
BRepTools.BRepTools().Read(shape, sys.argv[1], builder)

defl=float(sys.argv[2])

sf=ShapeFix_ShapeTolerance()
sf.SetTolerance(shape, defl)

inc=BRepMesh_IncrementalMesh(shape, defl);

stlwriter=StlAPI_Writer()
#stlwriter.SetRelativeMode()
print defl
stlwriter.SetASCIIMode(False)
stlwriter.SetRelativeMode(False)
#stlwriter.SetCoefficient(defl)
stlwriter.SetDeflection(defl)
stlwriter.Write(shape, sys.argv[3])

#    s=None
#    if (fname.endswith('igs')):
#     reader=IGESControl_Reader()
#     reader.ReadFile(sys.argv[1])
#     reader.TransferRoots()
#     s=reader.OneShape()
#    elif fname.endswith('stp'):
#     reader=STEPControl_Reader()
#     reader.ReadFile(sys.argv[1])
#     reader.TransferRoots()
#     s=reader.OneShape()

