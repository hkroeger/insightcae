#!/usr/bin/env python

#awk 'sub(/^PARAMETERSET>>> /,""){f=1} /<<<PARAMETERSET/{f=0} f' $*
import sys, re, os
re_start=re.compile("^PARAMETERSET>>> *([^ ]+) *([^ ]+)$")
re_end=re.compile("^<<<PARAMETERSET")
re_include=re.compile("^ *#include *\\\"(.*)\\\" *$")

filename=sys.argv[1]
pdlexe=sys.argv[2]
basename=os.path.splitext(os.path.basename(sys.argv[1]))[0]
f=open(filename, "r")
inside=False
out=None

print "Scanning", filename, "for PDL definitions"
generated=[]
expanded_glob=set([])

def copy(l, out, expanded):
  m=re.match(re_include, l)
  if not m is None:
    fn=m.group(1)
    if fn in expanded:
      raise Exception("Circular dependency detected: File "+fn+" has already been expanded!");
    else:
      expanded.add(fn)
      sf=open(fn, 'r')
      for sl in sf.readlines(): copy(sl, out, expanded)
      expanded.remove(fn)
  else:
    out.write(l);
  
for l in f.readlines():
  #print l
  
  if inside:
    if not re.match(re_end, l) is None:
      if not out is None: out.close()
      inside=False
      
  if inside:
    copy(l, out, expanded_glob)
    
  if not inside:
    m=re.search(re_start, l)    
    if not m is None:
      prefix=m.group(1)
      classname=m.group(2).rstrip()
      outfname=basename+"__"+prefix+"__"+classname+".pdl"
      print "Generating", outfname
      generated.append(outfname)
      out=open(outfname, "w")
      inside=True
      
      
for f in generated:
  os.system("\"%s\" \"%s\""%(pdlexe, f))