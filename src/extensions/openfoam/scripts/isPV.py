#!/usr/bin/env python

import os, sys, subprocess

def touch(fname, times=None):
    with file(fname, 'a'):
        os.utime(fname, times)
        
def split(more, path):
  head,tail=os.path.split(path)
  if ((head!="")) and (more>1):
    return split(more-1, head)+"_"+tail
  else:
    return tail
  
def writeloadscript(path, batch=False):
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
servermanager.LoadState("default.pvsm")

AnimationScene1 = GetAnimationScene()
AnimationScene1.EndTime = times[-1]
""")
  if batch:
    f.write("""\
for curtime in times:
  AnimationScene1.AnimationTime = curtime
  for i in range(0,len(GetRenderViews())):
    fname="screenshot_view%02d_t%g.png"%(i,curtime)
    print "Writing", fname
    WriteImage(fname, GetRenderViews()[i], Writer="vtkPNGWriter", Magnification=1)
""")
  else:
    f.write("AnimationScene1.AnimationTime = times[-1]\n")
    
  return fn
        
cn=split(3, os.getcwd())+".foam"
touch(cn)

batchrun=(sys.argv[-1]=='-b')

if (os.path.exists(os.path.join(os.getcwd(), "default.pvsm"))):
  scrp=writeloadscript(os.getcwd(), batchrun);
  if batchrun:
    subprocess.call(["pvbatch", "--use-offscreen-rendering", scrp])
  else:
    subprocess.call(["paraview", "--script="+scrp])
  os.remove(scrp)
else:
  subprocess.call(["paraview", "--data="+cn])

os.remove(cn)
