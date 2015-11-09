#!/usr/bin/env isRunSalomePvPython.sh

import os, sys, subprocess, pprint, re
from optparse import OptionParser

parser = OptionParser()
parser.add_option("-g", "--group", dest="group", metavar='FILE', default=[], action='append',
                  help="extract a group")

(opts, args) = parser.parse_args()

 
from paraview.simple import *


def extractGroup(
    infile, outfile,
    fields,
    groups,
    elno=True
  ):
  
  rmed = MEDReader(FileName=infile)
  rmed.GenerateVectors = 1
  rmed.AllArrays = ['%s@@][@@%s'%(fieldname,fieldtype) for fieldname, fieldtype in fields]

  extractGroup1 = ExtractGroup(Input=rmed)
  extractGroup1.AllGroups = ['GRP_%s'%gn for gn in groups]

  if (elno):
    eLNOMesh1 = ELNOMesh(Input=extractGroup1)
  else:
    eLNOMesh1 = extractGroup1

  mergeBlocks1 = MergeBlocks(Input=eLNOMesh1)

  SaveData(outfile, proxy=mergeBlocks1)
  
  Delete(mergeBlocks1)
  if (elno):
    Delete(eLNOMesh1)
  Delete(extractGroup1)
  Delete(rmed)
  del mergeBlocks1
  if (elno):
    del eLNOMesh1
  del extractGroup1
  del rmed
  
  
  
  
def extractCombinedVectorField(
   infile, outfile,
   fields, unif_name
  ):
  
  rmeds=[None] * len(fields)
  calcs=[None] * len(fields)
  
  for i,f in enumerate(fields):
    rmeds[i] = MEDReader(FileName=infile)
    rmeds[i].AllArrays = [f+'@@][@@P1']
    rmeds[i].GenerateVectors = 1

    calcs[i] = Calculator(Input=rmeds[i])
    calcs[i].ResultArrayName = unif_name
    calcs[i].Function = f.split('/')[-1]+'_Vector'

  groupDatasets1 = GroupDatasets(Input=calcs)
  mergeBlocks1 = MergeBlocks(Input=groupDatasets1)

  SaveData(outfile, proxy=mergeBlocks1)
  
  Delete(mergeBlocks1)
  Delete(groupDatasets1)
  for i in range(0, len(fields)):
    Delete(calcs[i])
    Delete(rmeds[i])

# example:
# PATH=$PATH:~/salome/appli_V7_6_0 isExtractVTKFromRMED.py -g hulk_ensdorf_exhaust.rmed:TS0/mesh/ComSup1/sol_____DEPL@P1:b3:test.vtk
for g in opts.group:
  print g.split(':')
  infile,fields,groups,outfile=g.split(':')
  extractGroup(
    infile, outfile,
    [fd.split('@') for fd in fields.split(',')],
    groups.split(',')
    )