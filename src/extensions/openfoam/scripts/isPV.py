#!/usr/bin/env python

import os, sys, subprocess, pprint, re, tempfile
from optparse import OptionParser

parser = OptionParser()
parser.add_option("-s", "--statefile", dest="statefile", metavar='FILE', default="",
                  help="load specified state file, search first in current dir then in insight shared dir")
parser.add_option("-u", "--unchanged", dest="statefileunchanged",
		  action='store_true',
                  help="skip the removal of absolute paths to foam case entries in statefile")
parser.add_option("-r", "--loadscript", dest="loadscript", metavar='FILE', default="",
                  help="include (append) the specified snippet into the loadscript")
parser.add_option("-b", "--batch", dest="batch",
		  action='store_true',
                  help="load specified state file, search first in current dir then in insight shared dir")
parser.add_option("-c", "--rescale", dest="rescale",
		  action='store_true',
                  help="automatically rescale all contour plots to data range (within each time step)")
parser.add_option("-f", "--from", dest="fromt", metavar="t0", default=0, type="float",
                  help="initial time")
parser.add_option("-t", "--to", dest="tot", metavar="t1", default=1e10, type="float",
                  help="final time")
parser.add_option("-a", "--onlylatesttime", dest="onlylatesttime",
		  action='store_true',
                  help="only select the latest time step  (overrides --to and --from, if they are given)")
parser.add_option("-l", "--list", dest="list",
		  action='store_true',
                  help="list available state files and exit, search first in current dir then in insight shared dir")

(opts, args) = parser.parse_args()

def touch(fname, times=None):
    with file(fname, 'a'):
        os.utime(fname, times)
        
def split(more, path):
  head,tail=os.path.split(path)
  if ((head!="")) and (more>1):
    return split(more-1, head)+"_"+tail
  else:
    return tail
  
casename = split(3, os.getcwd())

def writeloadscript(scrname, path, batch=False, loadcmd=True, appendFile=""):
  fn=os.path.join(os.getcwd(), ".loadscript.py")
  f=open(fn, "w")
  f.write("""\
try: paraview.simple
except: from paraview.simple import *

paraview.simple._DisableFirstRenderCameraReset()
import os,sys,re

#print os.getcwd()
def isfloat(x):
 try:
  float(x)
  return True;
 except:
  return False;

times=sorted(map(float, filter(isfloat, os.listdir("."))))
print times
#curtime=times[-1]
"""
+ ('paraview.simple.LoadState("%s")\n'%scrname if loadcmd else "") +
"""

AnimationScene1 = GetAnimationScene()
AnimationScene1.EndTime = times[-1]
""")
  
  if (appendFile!=""):
    for line in open(appendFile, 'r'):
      f.write(line)
      
  if batch:
    print opts.fromt, opts.tot # error occurs, when this statement is removed
#  for i in range(0,len(GetRenderViews())):
#    fname="%s_view%%02d_t%%g.png"%%(i,curtime)
#    print "Writing", fname
#    WriteImage(fname, GetRenderViews()[i], Writer="vtkPNGWriter", Magnification=1)

    rescalesnippet=""
    if (opts.rescale):
      rescalesnippet="""
  import math
  for view in GetRenderViews():
    reps = view.Representations
    for rep in reps:
     if hasattr(rep, 'Visibility') and rep.Visibility == 1 and hasattr(rep, 'MapScalars') and rep.MapScalars != '':
      input = rep.Input
      input.UpdatePipeline() #make sure range is up-to-date
      rep.RescaleTransferFunctionToDataRange(False)
"""
    f.write("""\
onlylatesttime=%s
ftimes=None
if (onlylatesttime):
 ftimes=[times[-1]]
else:
 ftimes=filter(lambda t: t>=%g and t<=%g, times)
for curtime in ftimes:
  AnimationScene1.AnimationTime = curtime
  
  #RescaleTransferFunctionToDataRange(False)
  
  RenderAllViews()
  
  %s
  
  layouts=GetLayouts()
  for i,l in enumerate(sorted(layouts.keys(), key=lambda k: k[0])):
  #for i,l in enumerate(layouts):
    fname="%s_layout%%02d"%%(i)
    if (not onlylatesttime):
      fname+="_t%%g.png"%%(curtime)
    else:
      fname+="_latesttime.png"
    print "Writing", fname
    SaveScreenshot(fname, layout=layouts[l], magnification=1, quality=100)
"""%(
  "True" if opts.onlylatesttime else "False",
  opts.fromt, opts.tot, rescalesnippet, casename))
  else:
    f.write("AnimationScene1.AnimationTime = times[-1]\n")
    
  return fn
        

searchdirs=map(lambda d: os.path.join(d, 'paraview'), [os.environ['INSIGHT_USERSHAREDDIR']]+os.environ['INSIGHT_GLOBALSHAREDDIRS'].split(':'))

if opts.list:
  avail=[]
  for d in searchdirs+['.']:
    if os.path.exists(d):
      avail+=filter(re.compile("^.*\\.pvsm$").search, os.listdir(d))
  pprint.pprint(avail)
  sys.exit(0)

# sed -e "s#value=\".*/\([^/]*\)\.foam\"#value=\"\\1\.foam\"#g" viz.pvsm
statefile=None

if (os.path.exists("default.pvsm")):
  statefile=os.path.join(os.getcwd(), "default.pvsm")
  
if (opts.statefile!=""):
  if not os.path.exists(opts.statefile):
    for d in searchdirs:
      sf=os.path.join(d, os.path.splitext(opts.statefile)[0]+".pvsm")
      if os.path.exists(sf):
	statefile=sf
	break
  else:
    statefile=os.path.abspath(opts.statefile)
    
  if statefile is None:
    print "Specified state file not found: ", opts.statefile
    sys.exit(-1)

remove_statefile=False
if not statefile is None and not opts.statefileunchanged:
  nsf,nsfname=tempfile.mkstemp(suffix='.pvsm', prefix='converted_statefile')
  subprocess.call(["sed", "-e", "s#value=\".*/\([^/]*\)\.foam\"#value=\"%s\.foam\"#g" % split(3, os.getcwd()), statefile], stdout=nsf)
  statefile=nsfname
  remove_statefile=True

if not statefile is None:
  if opts.batch:
    scrp=writeloadscript(statefile, os.getcwd(), opts.batch, loadcmd=True, appendFile=opts.loadscript);
    subprocess.call(["pvbatch", "--use-offscreen-rendering", scrp])
  else:
    scrp=writeloadscript(statefile, os.getcwd(), opts.batch, loadcmd=False, appendFile=opts.loadscript);
    subprocess.call(["paraview", "--state="+statefile, "--script="+scrp])
  os.remove(scrp)
else:
  cn=casename+".foam"
  touch(cn)
  subprocess.call(["paraview", "--data="+cn])
  os.remove(cn)
  
if remove_statefile:
  os.remove(statefile)
