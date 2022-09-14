#include "iqpointpickcommand.h"

#include "vtkRenderWindowInteractor.h"


IQPointPickCommand::IQPointPickCommand(
        vtkRenderWindowInteractor* rwi,
        const arma::mat&  pini )

  : rwi_(rwi),
    pickedPosition_(pini)
{
    picker_=decltype(picker_)::New();
    rwi_->SetPicker(picker_);

    rwi_->AddObserver( vtkCommand::EndPickEvent, this );
}




IQPointPickCommand::~IQPointPickCommand()
{
    rwi_->RemoveObserver(this);
    rwi_->SetPicker(nullptr);
}




void IQPointPickCommand::Execute(
        vtkObject *caller,
        unsigned long eventId,
        void */*callData*/ )
{
    if (eventId==vtkCommand::EndPickEvent)
    {
        picker_->GetPickPosition(pickedPosition_.memptr());
        Q_EMIT dataChanged();
    }
}




const arma::mat &IQPointPickCommand::getPickedPosition() const
{
    return pickedPosition_;
}
