#    This file is part of FOAMToolkit, a collection of python modules to
#    interface the OpenFOAM (R) Software
#    Copyright (C) 2010  Hannes Kroeger
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
from pprint import pprint

class Parameter:
  def __init__(self, ptype, ppath, pval):
    self.ptype=ptype
    self.ppath=ppath
    self.pval=pval
    
  def __call__(self):
    import importlib
    module = importlib.import_module('__builtin__')
    cls = getattr(module, self.ptype)
    return cls(self.pval)
    
  def __str__(self):
    return "--"+self.ptype+" "+self.ppath+":"+self.pval

def readParameters(fn='params.in'):
  os.system("cat params.in")
  params={}
  for l in open(fn, 'r').readlines():
    value,pv = l.strip().split()
    if ':' in pv:
      ptype,ppath = pv.split(':')
      params[ppath]=Parameter(ptype,ppath,value)
  pprint(params)
  return params

def writeQuality(qs, fn='results.out'):
  fo=open(fn, 'w')
  #for name,value in qs.items():
  for name in sorted(qs.keys()):
    print '%s = %g'%(name, qs[name])
    fo.write('%g %s\n'%(qs[name], name))
  fo.close()

def writeFail(fn='results.out'):
  fo=open(fn, 'w')
  fo.write('FAIL\n')
  fo.close()

def readSingleValueFile(fn):
  f=open(fn, 'r')
  val=float(f.readlines()[0])
  f.close()
  return val
