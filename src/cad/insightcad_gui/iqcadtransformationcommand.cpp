#include "iqcadtransformationcommand.h"

#include "vtkActor.h"
#include "vtkWidgetRepresentation.h"
#include "vtkTransform.h"
#include "vtkBoxRepresentation.h"
#include "vtkBoxWidget2.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindowInteractor.h"

IQCADTransformationCommand::IQCADTransformationCommand(
        vtkSmartPointer<vtkActor> actor,
        vtkRenderWindowInteractor* rwi,
        vtkTransform *tini )

  : actor_(actor),
    boxWidget_(vtkSmartPointer<vtkBoxWidget2>::New()),
    rwi_(rwi)
{
  boxWidget_->SetInteractor( rwi );
  boxWidget_->AddObserver( vtkCommand::InteractionEvent, this );
//  boxWidget_->AddObserver( vtkCommand::ExitEvent, this );
  boxWidget_->GetRepresentation()->SetPlaceFactor( 1.2 );
  boxWidget_->GetRepresentation()->PlaceWidget(actor_->GetBounds());
//  boxWidget_->RotationEnabledOff();
  boxWidget_->On();

  rwi->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(actor_);
  actor_->SetVisibility(true);

  auto repr = dynamic_cast<vtkBoxRepresentation*>( boxWidget_->GetRepresentation() );
  repr->SetTransform( tini );
  actor_->SetUserTransform( tini );
}




IQCADTransformationCommand::~IQCADTransformationCommand()
{
  actor_->SetVisibility(false);
  boxWidget_->Off();
  boxWidget_->RemoveAllObservers();
  rwi_->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->RemoveActor(actor_);
}




void IQCADTransformationCommand::Execute(
        vtkObject *caller,
        unsigned long eventId,
        void */*callData*/ )
{
    if (eventId==vtkCommand::InteractionEvent)
    {
        vtkSmartPointer<vtkBoxWidget2> boxWidget = dynamic_cast<vtkBoxWidget2*>(caller);
        auto repr = dynamic_cast<vtkBoxRepresentation*>( boxWidget->GetRepresentation() );

        double sfs[3], ra[3], tp[3];

        {
            auto t =  vtkSmartPointer<vtkTransform>::New();
            repr->GetTransform( t );
            t->GetScale(sfs);
            t->GetOrientation(ra);
            t->GetPosition(tp);
        }

        auto tmod =  vtkSmartPointer<vtkTransform>::New();
        tmod->Translate(tp);
        tmod->RotateZ(ra[2]);
        tmod->RotateX(ra[0]);
        tmod->RotateY(ra[1]);
        double sf = (sfs[0]+sfs[1]+sfs[2])/3.;
        tmod->Scale(sf, sf, sf);

        repr->SetTransform( tmod );
        actor_->SetUserTransform( tmod );

        Q_EMIT dataChanged();
    }

}

insight::SpatialTransformation IQCADTransformationCommand::getSpatialTransformation() const
{
    auto repr = dynamic_cast<vtkBoxRepresentation*>( boxWidget_->GetRepresentation() );
    auto t =  vtkSmartPointer<vtkTransform>::New();
    repr->GetTransform( t );
    return insight::SpatialTransformation(t);
}

