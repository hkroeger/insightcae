#!/usr/bin/env python

#awk 'sub(/^PARAMETERSET>>> /,""){f=1} /<<<PARAMETERSET/{f=0} f' $*
import sys, re, os
re_start=re.compile("^PARAMETERSET>>> *([^ ]+) *([^ ]+)$")
re_end=re.compile("^<<<PARAMETERSET")

filename=sys.argv[1]
pdlexe=sys.argv[2]
basename=os.path.splitext(os.path.basename(sys.argv[1]))[0]
f=open(filename, "r")
inside=False
out=None

print "Scanning", filename, "for PDL definitions"
generated=[]
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
      outfname=basename+"__"+prefix+"__"+classname+".pdl"
      print "Generating", outfname
      generated.append(outfname)
      out=open(outfname, "w")
      inside=True
      
for f in generated:
  os.system("\"%s\" \"%s\""%(pdlexe, f))