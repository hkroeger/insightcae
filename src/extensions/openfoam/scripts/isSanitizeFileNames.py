#!/usr/bin/env python
# -*- coding: utf-8 -*-

from optparse import OptionParser
import os, sys, shutil

parser = OptionParser()
parser.add_option("-k", "--keepcase", dest="keepcase", action='store_true',
                  help="don't convert to lower case file names")
parser.add_option("-r", "--rename", dest="rename", action='store_true',
                  help="rename files instead of copying")
parser.add_option("-f", "--force", dest="force", action='store_true',
                  help="overwrite target files, if they exist already")

(opts, args) = parser.parse_args()

replacements=[
    ('(', ''),
    (')', ''),
    ('[', ''),
    (']', ''),
    (' ', '_'),
    ('\\', ''),
    ('ä', 'ae'),
    ('ö', 'oe'),
    ('ü', 'ue'),
    ('/', '_'),
    ('"', '')
    ]

renames=[]

error=False
for fa in args:
    if (os.path.exists(fa)):
        fp=os.path.dirname(fa)
        fn=os.path.basename(fa)
        nfn=fn.lower() if not opts.keepcase else fn
        for r in replacements:
            nfn=nfn.replace(r[0], r[1])
        if (fn != nfn):
            nfa=os.path.join(fp, nfn)
            if (os.path.exists(nfa) and not opts.force):
                sys.stderr.write("Error: file with sanitized name already exists: "+nfa+"\n")
                error=True
            else:
                renames.append((fa, nfa))
                
for i in range(0, len(renames)):
    n=0
    for j in range(0, len(renames)):
        if (i!=j):
            if (renames[i][1]==renames[j][1]): n+=1
    if (n>0):
        sys.stderr.write("Error: more than one file would result in the same sanitized file name! (detected for "+renames[i][0]+")\n")
        error=True
            

            
if (error):
    sys.stderr.write("Exiting without changing any file.\n")
    exit(-1)
    
for r in renames:
    print( r[0], "=>", r[1])
    if (opts.rename):
        os.rename(r[0], r[1])
    else:
        shutil.copyfile(r[0], r[1])
        
