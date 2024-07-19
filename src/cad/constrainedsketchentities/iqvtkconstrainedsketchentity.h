#ifndef IQVTKCONSTRAINEDSKETCHENTITY_H
#define IQVTKCONSTRAINEDSKETCHENTITY_H


#include "constrainedsketchgeometry.h"

#include "vtkProp.h"

class IQVTKConstrainedSketchEntity
    : public insight::cad::ConstrainedSketchEntity
{
public:
    IQVTKConstrainedSketchEntity(const std::string& layerName=std::string());
    virtual std::vector<vtkSmartPointer<vtkProp> > createActor() const =0;
};


#endif // IQVTKCONSTRAINEDSKETCHENTITY_H
