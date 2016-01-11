#!/usr/bin/env python

import os, sys, subprocess, pprint, re, shutil
from optparse import OptionParser

parser = OptionParser()
#parser.add_option("-s", "--statefile", dest="statefile", metavar='FILE', default="",
                  #help="load specified state file, search first in current dir then in insight shared dir")
#parser.add_option("-r", "--loadscript", dest="loadscript", metavar='FILE', default="",
                  #help="include (append) the specified snippet into the loadscript")
#parser.add_option("-b", "--batch", dest="batch",
		  #action='store_true',
                  #help="load specified state file, search first in current dir then in insight shared dir")
#parser.add_option("-c", "--rescale", dest="rescale",
		  #action='store_true',
                  #help="automatically rescale all contour plots to data range (within each time step)")
#parser.add_option("-f", "--from", dest="fromt", metavar="t0", default=0, type="float",
                  #help="initial time")
#parser.add_option("-t", "--to", dest="tot", metavar="t1", default=1e10, type="float",
                  #help="final time")
#parser.add_option("-l", "--list", dest="list",
		  #action='store_true',
                  #help="list available state files and exit, search first in current dir then in insight shared dir")

(opts, args) = parser.parse_args()

rp=args[0]

if not os.path.exists(rp):
  if os.path.exists(os.path.join('processor0', rp)):
    os.system('reconstructPar -time '+rp)
  else:
    print "Error: time directory "+rp+" does not exist!"
    sys.exit(-1)

print "Creating restore point ", rp

for d in ['constant', 'system']:
  fns=os.listdir(d)
  for fn in fns:
    fnst=fn.rstrip('.gz')
    if fnst.endswith('Dict') or fnst.endswith('Properties') or fnst.startswith('fv'):
      fn=os.path.join(d, fn)
      targ=fn+"."+rp
      if os.path.exists(targ):
	print "Warning: overwriting "+targ+"!"
      shutil.copyfile(fn, targ)