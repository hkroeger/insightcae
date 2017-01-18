Name = 'UnwrapCylSlice'
Label = 'UnwrapCylSlice'
Help = 'Unwrap a propeller blade section'

NumberOfInputs = 1
InputDataType = 'vtkUnstructuredGrid'
OutputDataType = 'vtkUnstructuredGrid'
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

    numPoints = pdi.GetNumberOfPoints() 
    Urel=pdi.GetPointData().GetArray("Urel")


    # convert into numpy array for vector ops
    eax=np.array(eax) 
    er=np.array(er)
    etan=np.cross(eax,er)

    # copy fields into numpy lists
    p=np.array([pdi.GetPoint(i)[:3] for i in range(0, numPoints)]) -p0
    urel=np.array([Urel.GetTuple3(i) for i in range(0, numPoints)])

    r=np.sqrt(np.dot(p,er)**2+np.dot(p,etan)**2)
    z=np.dot(p,eax)
    theta=np.arctan2(np.dot(p,etan), np.dot(p,er))

    ct=np.cos(theta).reshape((numPoints,1))
    st=np.sin(theta).reshape((numPoints,1))

    o=np.ones((numPoints,3))
    er_local=(er*o)*ct + (etan*o)*st
    etan_local=(etan*o)*ct - (er*o)*st

    #print np.dot(urel,er_local)
    urel_flat = np.transpose( np.array([
        np.sum(urel*er_local,axis=1),
        np.sum(urel*etan_local,axis=1),
        np.dot(urel,eax)
        ]) )

    # create field for transformed points
    newPoints = vtk.vtkPoints() 

    # add new vector field
    Urf=vtk.vtkDoubleArray()
    Urf.SetName("Urel_flat")
    Urf.SetNumberOfComponents(3) 

    for i in range(0, numPoints):
        newPoints.InsertNextPoint(r[i], r[i]*theta[i], z[i])
        Urf.InsertNextTupleValue(urel_flat[i])

    pdo.SetPoints(newPoints)
    pdo.GetPointData().AddArray(Urf)
