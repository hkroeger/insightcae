#ifndef IQPOINTPICKCOMMAND_H
#define IQPOINTPICKCOMMAND_H

#include "toolkit_gui_export.h"

#include <armadillo>
#include <QObject>

#include "vtkSmartPointer.h"
#include "vtkCommand.h"
#include "vtkWorldPointPicker.h"

class vtkRenderWindowInteractor;

class TOOLKIT_GUI_EXPORT IQPointPickCommand
        : public QObject,
          public vtkCommand
{
    Q_OBJECT

    vtkSmartPointer<vtkWorldPointPicker> picker_;
    vtkRenderWindowInteractor* rwi_;

    arma::mat pickedPosition_;

  public:
    IQPointPickCommand(
        vtkRenderWindowInteractor* rwi,
        const arma::mat&  pini );

    ~IQPointPickCommand();

    void Execute( vtkObject *caller, unsigned long, void* ) override;

    const arma::mat& getPickedPosition() const;

  Q_SIGNALS:
    void dataChanged();
    void done();
};


#endif // IQPOINTPICKCOMMAND_H
