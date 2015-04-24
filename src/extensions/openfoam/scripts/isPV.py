#!/usr/bin/env python

import os, sys, subprocess, pprint, re
from optparse import OptionParser

parser = OptionParser()
parser.add_option("-s", "--statefile", dest="statefile", metavar='FILE', default="",
                  help="load specified state file, search first in current dir then in insight shared dir")
parser.add_option("-b", "--batch", dest="batch",
		  action='store_true',
                  help="load specified state file, search first in current dir then in insight shared dir")
parser.add_option("-f", "--from", dest="fromt", metavar="t0", default=0, type="float",
                  help="initial time")
parser.add_option("-t", "--to", dest="tot", metavar="t1", default=1e10, type="float",
                  help="final time")
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
  
def writeloadscript(scrname, path, batch=False):
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
servermanager.LoadState("%s")

AnimationScene1 = GetAnimationScene()
AnimationScene1.EndTime = times[-1]
"""%scrname)
  if batch:
    print opts.fromt, opts.tot # error occurs, when this statement is removed
    f.write("""\
for curtime in filter(lambda t: t>=%g and t<=%g, times):
  AnimationScene1.AnimationTime = curtime
  for i in range(0,len(GetRenderViews())):
    fname="screenshot_view%%02d_t%%g.png"%%(i,curtime)
    print "Writing", fname
    WriteImage(fname, GetRenderViews()[i], Writer="vtkPNGWriter", Magnification=1)
"""%(opts.fromt, opts.tot))
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



if not statefile is None:
  scrp=writeloadscript(statefile, os.getcwd(), opts.batch);
  if opts.batch:
    subprocess.call(["pvbatch", "--use-offscreen-rendering", scrp])
  else:
    subprocess.call(["paraview", "--script="+scrp])
  os.remove(scrp)
else:
  cn=split(3, os.getcwd())+".foam"
  touch(cn)
  subprocess.call(["paraview", "--data="+cn])
  os.remove(cn)
