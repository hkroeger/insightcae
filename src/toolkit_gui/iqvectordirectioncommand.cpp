#include "iqvectordirectioncommand.h"

#include "vtkLineRepresentation.h"
#include "vtkRenderWindowInteractor.h"

#include "base/linearalgebra.h"

IQVectorDirectionCommand::IQVectorDirectionCommand(
        vtkRenderWindowInteractor* rwi,
        const arma::mat& pBase,
        const arma::mat& vector )
    : lineWidget_(decltype(lineWidget_)::New()),
      rwi_(rwi),
      pBase_(pBase)
{
    lineWidget_->CreateDefaultRepresentation();
    lineWidget_->SetInteractor( rwi );
    lineWidget_->AddObserver( vtkCommand::InteractionEvent, this );

    double p[3];

    auto *line = lineWidget_->GetLineRepresentation();
    for (int j=0; j<3; ++j) p[j]=pBase[j];
    line->SetPoint1WorldPosition(p);

    for (int j=0; j<3; ++j) p[j]=pBase[j]+vector[j];
    line->SetPoint2WorldPosition(p);

    lineWidget_->On();
}


IQVectorDirectionCommand::~IQVectorDirectionCommand()
{
    lineWidget_->Off();
    lineWidget_->RemoveAllObservers();
}




void IQVectorDirectionCommand::Execute(
        vtkObject *caller,
        unsigned long eventId,
        void */*callData*/ )
{
    if (eventId==vtkCommand::InteractionEvent)
    {
        auto repr = dynamic_cast<vtkLineRepresentation*>(
                      lineWidget_->GetRepresentation() );

        lineWidget_->GetLineRepresentation()
                ->SetPoint1WorldPosition(arma::mat(pBase_).memptr());
        arma::mat pnew=insight::vec3Zero();
        lineWidget_->GetLineRepresentation()
                ->GetPoint2WorldPosition(pnew.memptr());

        vector_ = pnew - pBase_;

        Q_EMIT dataChanged();
    }

}

const arma::mat& IQVectorDirectionCommand::getVector() const
{
    return vector_;
}
