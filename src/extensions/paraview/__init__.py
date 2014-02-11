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

try:
    import os, re, copy
    from paraview.simple import *
    
    
    def loadOFCase(caseDir):
        import re
    
        case = OpenFOAMReader(FileName = os.path.join(caseDir, "x.foam"))
        bnds=case.GetPropertyValue('PatchArrayInfo')
        
        regions=['internalMesh']+[bnds[i] for i in range(0, len(bnds), 2)]
        
        blockIndices={regions[0]: 1}
        blockIndices.update({n: 2+i for i,n in enumerate(regions[1:])})
        
        #print blockIndices
        case.MeshRegions=regions
        
        vs=case.TimestepValues
        view=GetActiveView()
        view.ViewTime=vs[-1]
        
        return (case, blockIndices)
    
    
    lookupTableTemplates={
    'bluered': {'RGBPoints': [
                    0.0,    0.0,    0.333333,    1,
                    0.2003664868,    0.576471,    1,    0.988235,
                    0.4172793444,    0.0,    0.870588,    0.0862745,
                    0.6102942075,    0.976471,    0.94902,    0.0980392,
                    0.8180138451,    1.0,    0.0,    0.0,
                    1.0,    1.0,    0.0,    1.0
                    ], 
                'ColorSpace': "RGB"},
    'water': {'RGBPoints': [                    
                    0.0,    0,    0,    0,
                    0.1604691667,    0.00784314,    0.0627451,    0.517647,
                    0.3907941667,    0.2,    0.592157,    0.984314,
                    0.5794225,    0.0509804,    0.933333,    1,
                    1.0,    1,    1,    1
                    ], 
                'ColorSpace': "RGB"}
    }
    
    
    def getLookupTable(arrayName, minV, maxV, component, template):
        p=copy.copy(template)
        for i in range(0, len(p['RGBPoints'])/4):
            p['RGBPoints'][4*i]=minV+(maxV-minV)*p['RGBPoints'][4*i]
        #print p
        return GetLookupTableForArray(arrayName, component, **p)
    
    
    def displayContour(obj, arrayName, minV, maxV, component=-1, LUTName="bluered", 
                       title=None, barpos=[0.75, 0.25], barorient=1):
        disp = GetDisplayProperties(obj)
        #disp.LookupTable=MakeBlueToRedLT(-1, 1)
        disp.LookupTable=getLookupTable(arrayName, minV, maxV, 
                                        1 if component<0 else component, 
                                        lookupTableTemplates[LUTName])
        if component>=0:
            disp.LookupTable.VectorComponent=component
            disp.LookupTable.VectorMode="Component"
        disp.Representation = 'Surface'
        disp.ColorArrayName=arrayName
        disp.ColorAttributeType='POINT_DATA'
            
        bar = CreateScalarBar(
                              LookupTable=disp.LookupTable, 
                              Title=(arrayName if title is None else title),
                              Position=barpos, Orientation=barorient,
                              TitleFontSize=14, LabelFontSize=12,
                              TitleColor=[0,0,0], LabelColor=[0,0,0]
                              )
        GetRenderView().Representations.append(bar)
    
    
    
    def extractPatches(cbi, patches):
        case, blockIndices=cbi
        
        eb=ExtractBlock(Input=case, PruneOutput=1)
        if isinstance(patches, list):
            eb.BlockIndices=[blockIndices[k] for k in patches]
        elif isinstance(patches, str):
            se=re.compile(patches)
            eb.BlockIndices=[i for n,i in blockIndices.items() if se.match(n)]
        else:
            raise Exception("no valid patch selection given! Specify either a string list or a single regex string")
        
        return eb
    
    
    def waterSurface(cbi, minZ, maxZ):
        case, blockIndices=cbi
        
        surf=Contour(Input=case, ContourBy='alpha1', Isosurfaces=[0.5])
        elev=Elevation(Input=surf, 
                       LowPoint=[0,0,minZ], 
                       HighPoint=[0,0,maxZ], 
                       ScalarRange=[minZ, maxZ]
                       )
        
        return elev
    
    def setCam(pos, focus=[0,0,0], up=[0,0,1], scale=1.):
        cam = GetActiveCamera()
        cam.ParallelProjectionOn()
        cam.SetParallelScale(scale)
        cam.SetViewUp(up)
        cam.SetFocalPoint(focus)
        cam.SetPosition(pos)
        
    def prepareSnapshots():
        paraview.simple._DisableFirstRenderCameraReset()
        active_objects.source.SMProxy.InvokeEvent('UserEvent', 'HideWidget')

except ImportError:
    pass

