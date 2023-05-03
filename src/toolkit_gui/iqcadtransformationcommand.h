#ifndef IQCADTRANSFORMATIONCOMMAND_H
#define IQCADTRANSFORMATIONCOMMAND_H


#include <QObject>

#include "vtkCommand.h"
#include "vtkSmartPointer.h"

#include "base/spatialtransformation.h"

class vtkActor;
class vtkBoxWidget2;
class vtkRenderWindowInteractor;
class vtkLinearTransform;


class IQCADTransformationCommand
        : public QObject,
          public vtkCommand
{
    Q_OBJECT

    vtkSmartPointer<vtkActor> actor_;
    vtkSmartPointer<vtkBoxWidget2> boxWidget_;
    vtkRenderWindowInteractor* rwi_;

  public:
    IQCADTransformationCommand(
        vtkSmartPointer<vtkActor> actor,
        vtkRenderWindowInteractor* rwi,
        vtkTransform *tini );

    ~IQCADTransformationCommand();

    void Execute( vtkObject *caller, unsigned long, void* ) override;

    insight::SpatialTransformation getSpatialTransformation() const;

  Q_SIGNALS:
    void dataChanged();
    void done();
};

#endif // IQCADTRANSFORMATIONCOMMAND_H
