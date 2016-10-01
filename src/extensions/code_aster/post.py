
import os, sys, csv, re, numpy


class AsterTable:
  
  def __init__(self, fname, tabname):
    f=open(fname, 'r')
    lines=f.readlines()
    
    start=re.compile('##ASTER .+\\..+\\..+ CONCEPT (.+) CALCULE LE ../../.... A ..:..:.. DE TYPE')

    i0=None
    # search 
    for i,l in enumerate(lines):
      ma=start.match(l)
      if not ma is None:
        if ma.group(1)==tabname:
            i0=i
            break
	
    if i0 is None:
      raise Exception("table "+tabname+" not found in file "+fname)
      
    self.headers=[f.strip() for f in lines[i0+2].split()]

    def tryNum(x):
        try:
            return float(x)
        except:
            return x
      
    fields=[]
    for i,l in enumerate(lines[i0+3:]):
      if not l.strip():
        break
      else:
        fields.append([l[:17].strip()] + map(tryNum, l[17:].split()))

    self.fields=fields
    
    #print self.headers
    #print self.fields
    
    
  def col(self, colname):
    j=self.headers.index(colname)
    return numpy.array([self.fields[i][j] for i in range(0,len(self.fields))])
  
