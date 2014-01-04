#!/usr/bin/python

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
        
cn=split(3, os.getcwd())+".foam"
touch(cn)
subprocess.call(["paraview", "--data="+cn])
os.remove(cn)
