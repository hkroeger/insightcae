
import FreeCAD, FreeCADGui, Part
import Insight_rc
import InsightFeatures


class todo:
    ''' static todo class, delays execution of functions.  Use todo.delay
    to schedule geometry manipulation that would crash coin if done in the
    event callback'''

    '''List of (function, argument) pairs to be executed by
    QtCore.QTimer.singleShot(0,doTodo).'''
    itinerary = []
    commitlist = []
    
    @staticmethod
    def doTasks():
        # print "debug: doing delayed tasks: commitlist: ",todo.commitlist," itinerary: ",todo.itinerary
        for f, arg in todo.itinerary:
            try:
                # print "debug: executing",f
                if arg:
                    f(arg)
                else:
                    f()
            except:
                wrn = "[Draft.todo.tasks] Unexpected error:", sys.exc_info()[0], "in ", f, "(", arg, ")"
                FreeCAD.Console.PrintWarning (wrn)
        todo.itinerary = []
        if todo.commitlist:
            for name,func in todo.commitlist:
                # print "debug: committing ",str(name)
                try:
                    name = str(name)
                    FreeCAD.ActiveDocument.openTransaction(name)
                    if isinstance(func,list):
                        for l in func:
                            FreeCADGui.doCommand(l)
                    else:
                        func()
                    FreeCAD.ActiveDocument.commitTransaction()
                except:
                    wrn = "[Draft.todo.commit] Unexpected error:", sys.exc_info()[0], "in ", f, "(", arg, ")"
                    FreeCAD.Console.PrintWarning (wrn)
            # restack Draft screen widgets after creation
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.restack()
        todo.commitlist = []

    @staticmethod
    def delay (f, arg):
        # print "debug: delaying",f
        if todo.itinerary == []:
            QtCore.QTimer.singleShot(0, todo.doTasks)
        todo.itinerary.append((f,arg))

    @staticmethod
    def delayCommit (cl):
        # print "debug: delaying commit",cl
        QtCore.QTimer.singleShot(0, todo.doTasks)
        todo.commitlist = cl


class ExtractCurve: 

   #def Activated(self): 
      #sel=FreeCADGui.Selection.getSelectionEx()
      #edges = [ e for so in sel for e in so.SubObjects ]
      #FreeCAD.Console.PrintMessage('Exporting %d elements into file'%len(edges))
      #w=Part.Wire(edges)
      #Part.show(w)

  def Activated(self):
      InsightFeatures.makeBox()

  def GetResources(self): 
      return {
	'Pixmap' : 'Draft_Arc', 
	'MenuText': 'Extract curve', 
	'ToolTip': 'Extract Curve and write to brep file'
	} 

class EdgeSelectionUi:
  def __init__(self):
    todo.delay(FreeCADGui.Control.closeDialog, None)
    self.baseWidget = QtGui.QWidget()
    self.layout = QtGui.QVBoxLayout(self.baseWidget)
    self.panel = DraftTaskPanel(self.baseWidget, extra)
    todo.delay(FreeCADGui.Control.showDialog, self.panel)
    

class ExtractedWire:
    "The PathArray FreeCAD command definition"

    def Activated(self):
      if not FreeCADGui.Selection.getSelectionEx():
	#msg(translate("draft", "Please select base and path objects\n"))
	self.ui = EdgeSelectionUi()
      else:
	sel = FreeCADGui.Selection.getSelectionEx()
	if sel:
	  print sel
	  edgesobject = sel[0].Object
	  edges = list(sel[0].SubElementNames)
	  FreeCAD.ActiveDocument.openTransaction("ExtractedWire")
	  InsightFeatures.makeExtractedWire(edgesobject, edges)
	  FreeCAD.ActiveDocument.commitTransaction()

    def GetResources(self):
        return {
	'Pixmap' : 'Draft_Arc', 
	'MenuText': 'Extracted Wire', 
	'ToolTip': 'Extracted Wire'
	  }

	      
FreeCADGui.addCommand('Insight_ExtractCurve', ExtractCurve())
FreeCADGui.addCommand('Insight_ExtractedWire', ExtractedWire())
