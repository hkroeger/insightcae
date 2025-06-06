#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os, sys, subprocess, pprint, re, tempfile, glob, shutil
from optparse import OptionParser
import Insight.vtkPyOffscreen
from Insight.toolkit import *

parser = OptionParser()
parser.disable_interspersed_args()
parser.add_option("-d", "--no-decomp", dest="nodecomp",
                  action='store_true',
                  help="skip the decomposition prior to parallel execution")
parser.add_option("-f", "--field-decomp", dest="fielddecomp",
                  action='store_true',
                  help="decompose fields only, not the mesh")
parser.add_option("-r", "--no-reconst", dest="noreconst",
                  action='store_true',
                  help="skip the reconstruction after parallel execution")
parser.add_option("-l", "--reconst-only-latesttime", dest="reconstonlylatesttime",
                  action='store_true',
                  help="reconstruct only the latest time step")
parser.add_option("-m", "--mesh-reconst", dest="meshreconst",
                  action='store_true',
                  help="execute reconstructParMesh instead of reconstructPar for reconstruction")
parser.add_option("--remove-processordirs", dest="removeproc",
                action='store_true',
                help="remove processor directories (after reconstruct, if applicable)")
parser.add_option("-q", "--queue", dest="queue",
                  action='store_true',
                  help="schedule the executed commands through task spooler")
parser.add_option("-c", "--case", dest="case", metavar='CASE-DIRECTORY', default=".",
                  help="folder containing the OpenFOAM case")
parser.add_option("-i", "--mirun-cmd", dest="mpiruncmd", metavar='EXECUTABLE', default="mpirun",
                  help="use the specified prefix for parallel runs")

(opts, args) = parser.parse_args()

is_parallel=False;
np=1
case=opts.case

try:
    np=readDecomposeParDict(case)
    if (np>1):
        is_parallel=True
except:
    pass

if is_parallel and not opts.nodecomp:

    # check, if decomp is really needed
    #d=decompositionState(case)
    onlyfielddecomp=opts.fielddecomp
    #if d.hasProcessorDirectories and d.nProcDirsMatchesDecomposeParDict and d.decomposedLatestTimeIsConsistent

    subprocess.call(
      ["decomposePar"] + (["-fields"] if onlyfielddecomp  else []),
      cwd=case
      )

mpirun=opts.mpiruncmd
solver=readSolverName(case)

cmd = [solver]
if len(args)>0:
    cmd = args

if is_parallel:
    cmd = [mpirun, '-np', '%d'%np] + cmd + ['-parallel']

subprocess.call(cmd, cwd=case)

if is_parallel and not opts.noreconst:
    subprocess.call(
      (["reconstructPar"] if not opts.meshreconst else ["reconstructParMesh", "-constant"])
      +
      (["-latestTime"] if opts.reconstonlylatesttime else []),
      cwd=case
      )

if is_parallel and opts.removeproc:
    for pd in glob.glob(os.path.join(case, "processor*")):
        shutil.rmtree(pd)

