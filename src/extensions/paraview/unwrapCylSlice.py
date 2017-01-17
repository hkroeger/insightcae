Name = 'UnwrapCylSlice'
Label = 'UnwrapCylSlice'
Help = 'Unwrap a propeller blade section'

NumberOfInputs = 1
InputDataType = 'vtkPolyData'
OutputDataType = 'vtkPolyData'
ExtraXml = ''


Properties = dict(
  p0 = [0., 0., 0.],
  eax = [0., 0., 1.],
  er = [1., 0., 0.],
  )


def RequestData():

    import numpy as np  
 
    pdi = self.GetInputDataObject(0,0)
    pdo = self.GetOutputDataObject(0)
    pdo.ShallowCopy(pdi)

    newPoints = vtk.vtkPoints() 
    numPoints = pdi.GetNumberOfPoints() 
    Urel=pdi.GetPointData().GetArray("Urel")

    Urf=vtk.vtkDoubleArray()
    Urf.SetName("Urel_flat")
    Urf.SetNumberOfComponents(3) 

    def nc(x,y,z):
        r=np.sqrt(x*x+y*y)
        t=np.arctan2(y,x)
        z=z
        return r,t,z

    for i in range(0, numPoints): 
        coord = pdi.GetPoint(i) 
        x, y, z = coord[:3] 
        r,t,z=nc(x,y,z) 
        newPoints.InsertNextPoint(r,r*t,z) 
        ux,uy,uz=Urel.GetTuple3(i)
        Urf.InsertNextTupleValue([
            ux*np.cos(-t) - uy*np.sin(-t),
            ux*np.sin(-t) + uy*np.cos(-t),
            uz
            ])

    pdo.SetPoints(newPoints)
    pdo.GetPointData().AddArray(Urf)
