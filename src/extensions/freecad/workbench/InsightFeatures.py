
import FreeCAD, FreeCADGui, Part
from pivy import coin

class Box:
	def __init__(self, obj):
		"Add some custom properties to our box feature"
		obj.addProperty("App::PropertyLength","Length","Box","Length of the box").Length=1.0
		obj.addProperty("App::PropertyLength","Width","Box","Width of the box").Width=1.0
		obj.addProperty("App::PropertyLength","Height","Box", "Height of the box").Height=1.0
		obj.Proxy = self
	
	def onChanged(self, fp, prop):
		"Do something when a property has changed"
		FreeCAD.Console.PrintMessage("Change property: " + str(prop) + "\n")
		#self.execute(fp)
 
	def execute(self, fp):
		"Do something when doing a recomputation, this method is mandatory"
		FreeCAD.Console.PrintMessage("Recompute Python Box feature\n")
		cube = Part.makeBox(fp.Length, fp.Width, fp.Height)
		fp.Shape = cube

	      
def makeBox():
	a=FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Box")
	Box(a)
 	a.ViewObject.Proxy=0
 	FreeCAD.ActiveDocument.recompute()



class ExtractedWire:
	def __init__(self, obj):
	    "Add some custom properties to our box feature"
	    obj.addProperty("App::PropertyLink","edgesobject","Extracted Wire",
			    "Object, where the edges are to be extracted from")
	    obj.addProperty("App::PropertyLinkSubList", "edges", "Extracted Wire", "Edges to be extracted")
	    obj.edges=[]
	    obj.Proxy = self
	
	def onChanged(self, fp, prop):
	    "Do something when a property has changed"
	    FreeCAD.Console.PrintMessage("Change property: " + str(prop) + "\n")
	    #self.execute(fp)

	def getWireFromSubs(self, fp):
	    '''Make a wire from PathObj subelements'''
	    sl = []
	    for sub in fp.edges:
		e = sub[0].Shape.getElement(sub[1])
		sl.append(e)
	    return Part.Wire(sl)
 
	def execute(self, fp):
	    "Do something when doing a recomputation, this method is mandatory"
	    print fp
	    FreeCAD.Console.PrintMessage("Recompute Python Box feature\n")
	    w = self.getWireFromSubs(fp)
	    fp.Shape = w

def makeExtractedWire(edgesobject, edges=[]):
	print edgesobject, edges
	a=FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "ExtractedWire")
	ExtractedWire(a)
	a.edgesobject=edgesobject
	if edges:
	    sl = []
	    for sub in edges:
		sl.append((a.edgesobject,sub))
	    a.edges = list(sl)

 	a.ViewObject.Proxy=0
 	FreeCAD.ActiveDocument.recompute()
 	
 	
