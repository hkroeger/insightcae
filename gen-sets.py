#!/usr/bin/env python

#awk 'sub(/^PARAMETERSET>>> /,""){f=1} /<<<PARAMETERSET/{f=0} f' $*
import sys, re
re_start=re.compile("^PARAMETERSET>>> *([^ ]+) *([^ ]+)$")
re_end=re.compile("^<<<PARAMETERSET")

f=open(sys.argv[1], "r")
inside=False
out=None

for l in f.readlines():
  #print l
  
  if inside:
    if not re.match(re_end, l) is None:
      if not out is None: out.close()
      inside=False
      
  if inside:
    out.write(l);
    
  if not inside:
    m=re.search(re_start, l)    
    if not m is None:
      prefix=m.group(1)
      classname=m.group(2).rstrip()
      outfname=prefix+"__"+classname+".pdl"
      print "Generating", outfname
      out=open(outfname, "w")
      inside=True