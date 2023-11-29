#include "iqcadtransformationcommand.h"

#include "vtkActor.h"
#include "vtkWidgetRepresentation.h"
#include "vtkTransform.h"
#include "vtkBoxRepresentation.h"
#include "vtkBoxWidget2.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindowInteractor.h"

vtkSmartPointer<vtkTransform> IQCADTransformationCommand::constrainedTransform(vtkTransform *t) const
{
    arma::mat sfs, ra, tp;
    sfs=ra=tp=insight::vec3Zero();


    if (lockScale_) tini_->GetScale(sfs.memptr()); else t->GetScale(sfs.memptr());
    if (lockRotation_) tini_->GetOrientation(ra.memptr()); else t->GetOrientation(ra.memptr());
    if (lockTranslation_) tini_->GetPosition(tp.memptr()); else t->GetPosition(tp.memptr());

    auto tmod =  vtkSmartPointer<vtkTransform>::New();
    tmod->Translate(tp.memptr());
    tmod->RotateZ(ra[2]);
    tmod->RotateX(ra[0]);
    tmod->RotateY(ra[1]);
    double sf = (sfs[0]+sfs[1]+sfs[2])/3.;
    tmod->Scale(sf, sf, sf);

    return tmod;
}

IQCADTransformationCommand::IQCADTransformationCommand(
        vtkSmartPointer<vtkActor> actor,
        vtkRenderWindowInteractor* rwi,
        vtkTransform *tini,
    bool lockScale,
    bool lockRotation,
    bool lockTranslation )

  : actor_(actor),
    boxWidget_(vtkSmartPointer<vtkBoxWidget2>::New()),
    rwi_(rwi),
    tini_(vtkSmartPointer<vtkTransform>::New()),
    lockScale_(lockScale),
    lockRotation_(lockRotation),
    lockTranslation_(lockTranslation)
{
  tini_->DeepCopy(tini);

  boxWidget_->SetInteractor( rwi );
  boxWidget_->AddObserver( vtkCommand::InteractionEvent, this );
  boxWidget_->GetRepresentation()->SetPlaceFactor( 1.2 );
  boxWidget_->GetRepresentation()->PlaceWidget(actor_->GetBounds());
  boxWidget_->SetRotationEnabled(!lockRotation_);
  boxWidget_->SetScalingEnabled(!lockScale_);
  boxWidget_->SetTranslationEnabled(!lockTranslation_);
  boxWidget_->On();


  rwi->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(actor_);
  actor_->SetVisibility(true);

  auto repr = dynamic_cast<vtkBoxRepresentation*>( boxWidget_->GetRepresentation() );
  repr->SetTransform( tini_ );
  actor_->SetUserTransform( tini_ );
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
        auto boxWidget = dynamic_cast<vtkBoxWidget2*>(caller);
        auto repr = dynamic_cast<vtkBoxRepresentation*>( boxWidget->GetRepresentation() );

        auto t =  vtkSmartPointer<vtkTransform>::New();
        repr->GetTransform( t );

        auto tmod =  constrainedTransform(t);

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
    return insight::SpatialTransformation(constrainedTransform(t));
}

