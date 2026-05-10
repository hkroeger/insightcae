#!/usr/bin/env python3

#awk 'sub(/^PARAMETERSET>>> /,""){f=1} /<<<PARAMETERSET/{f=0} f' $*
import sys, re, os, shutil, glob
re_start=re.compile("^PARAMETERSET>>> *([^ ]+) *([^ ]+)$")
re_end=re.compile("^<<<PARAMETERSET")
re_include=re.compile("^ *#include *\\\"(.*)\\\" *$")

filename=sys.argv[1]
pdlexe=sys.argv[2]
addlocs=sys.argv[3:] if len(sys.argv)>3 else []
basename=os.path.splitext(os.path.basename(sys.argv[1]))[0]
f=open(filename, "r")
inside=False
out=None

print("Scanning", filename, "for PDL definitions")
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

# Read manifest from previous run to detect stale files
manifest_file = basename + "_pdl.manifest"
try:
    with open(manifest_file) as mf:
        previous_stems = set(line for line in mf.read().splitlines() if line)
except FileNotFoundError:
    previous_stems = set()

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
      outfname=basename+"__"+prefix+"__"+classname
      print("Generating", outfname)
      generated.append(outfname)
      out=open(outfname+".pdl", "w")
      inside=True


for f in generated:
  if os.system("\"%s\" \"%s\""%(pdlexe, f+".pdl"))!=0:
      sys.exit(-1)
  for file in glob.glob(f+"__*.h"):
      for al in addlocs:
          shutil.copy(file, al)

# Remove stale files from previous runs whose stems are no longer generated
current_stems = set(generated)
for stale_stem in previous_stems - current_stems:
    stale_pdl = stale_stem + ".pdl"
    if os.path.exists(stale_pdl):
        print("Removing stale PDL:", stale_pdl)
        os.remove(stale_pdl)
    for stale_h in glob.glob(stale_stem + "__*.h"):
        print("Removing stale header:", stale_h)
        os.remove(stale_h)
        for al in addlocs:
            mirror = os.path.join(al, os.path.basename(stale_h))
            if os.path.exists(mirror):
                print("Removing stale header from addloc:", mirror)
                os.remove(mirror)

# Write updated manifest
with open(manifest_file, "w") as mf:
    mf.write("\n".join(generated))
