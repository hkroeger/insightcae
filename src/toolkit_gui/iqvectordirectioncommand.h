#ifndef IQVECTORDIRECTIONCOMMAND_H
#define IQVECTORDIRECTIONCOMMAND_H

#include <armadillo>
#include <QObject>

#include "vtkCommand.h"
#include "vtkSmartPointer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkLineWidget2.h"


class IQVectorDirectionCommand
        : public QObject,
          public vtkCommand
{
    Q_OBJECT

    vtkSmartPointer<vtkLineWidget2> lineWidget_;
    vtkRenderWindowInteractor* rwi_;

    arma::mat pBase_, vector_;

public:
    IQVectorDirectionCommand(
            vtkRenderWindowInteractor* rwi,
            const arma::mat& pBase,
            const arma::mat& vector);

    ~IQVectorDirectionCommand();

    void Execute( vtkObject *caller, unsigned long, void* ) override;

    const arma::mat& getVector() const;

Q_SIGNALS:
    void dataChanged();
    void done();
};

#endif // IQVECTORDIRECTIONCOMMAND_H
