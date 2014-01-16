
import FreeCAD, FreeCADGui, Part
import Insight_rc

class ExtractCurve: 

#   def __init__(self, obj):
#      obj.Proxy = self
#      self.Type = "ExtractCurve"
#      obj.addProperty("App::PropertyLinkSub",
#                      "Edges",
#                      "Insight",
#                      "The edges to be extracted")

#   def createGeometry(self, obj):
#      edges = [ e for so in obj.Edges for e in so.SubObjects ]
#      w=Part.Wire(edges)
#      obj.Shape=w
      
#   def execute(self,obj):
#      self.createGeometry(obj)
    
   def Activated(self): 
      sel=FreeCADGui.Selection.getSelectionEx()
      edges = [ e for so in sel for e in so.SubObjects ]
      FreeCAD.Console.PrintMessage('Exporting %d elements into file'%len(edges))
      w=Part.Wire(edges)
      Part.show(w)

   def GetResources(self): 
       return {
          'Pixmap' : 'Draft_Arc', 
          'MenuText': 'Extract curve', 
          'ToolTip': 'Extract Curve and write to brep file'
          } 

FreeCADGui.addCommand('Insight_ExtractCurve', ExtractCurve())
