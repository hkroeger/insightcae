
#include "iqvtkcadmodel3dviewer.h"
#include "iqcaditemmodel.h"

#include "vtkProperty.h"


IQVTKCADModel3DViewer::HighlightItem::HighlightItem(
        std::shared_ptr<DisplayedEntity> de,
        QPersistentModelIndex idx2highlight,
        IQVTKCADModel3DViewer* viewer )
    : viewer_(viewer),
      de_(de),
      idx2highlight_(idx2highlight)
{
    for (auto& o: viewer_->displayedData_)
    {
        for (const auto& actor: o.second.actors_)
        {
            if ( auto act = vtkActor::SafeDownCast(/*o.second.actor_*/actor) )
            {
                if (o.first == idx2highlight_)
                {
                    act->GetProperty()->SetOpacity(1.);
                    act->GetProperty()->SetColor(1,0,0);
                    act->SetVisibility(true);
                }
                else
                {
                    act->GetProperty()->SetOpacity(0.1);
                }
            }
        }
    }

    if (de_)
    {
        for (const auto& actor: de->actors_)
        {
            if ( auto act = vtkActor::SafeDownCast(/*de_->actor_*/actor) )
            {
                act->GetProperty()->SetOpacity(1.);
                act->GetProperty()->SetColor(1,0,0);
            }
            viewer_->ren_->AddActor( /*de_->actor_ */ actor);
        }
    }

    viewer_->scheduleRedraw();
}




IQVTKCADModel3DViewer::HighlightItem::~HighlightItem()
{
    for (auto& o: viewer_->displayedData_)
    {
        for (const auto& actor: o.second.actors_)
        {
            if (auto act = vtkActor::SafeDownCast(/*o.second.actor_*/actor))
            {
                if ( o.first == idx2highlight_)
                {
                    // restore all display props
                    viewer_->resetDisplayProps(o.first);
                }
                else
                {
                    // restore opacity
                    auto opacity = QModelIndex(o.first)
                            .siblingAtColumn(IQCADItemModel::entityOpacityCol)
                            .data()
                            .toDouble();
                    act->GetProperty()->SetOpacity(opacity);
                }
            }
        }
    }

    if (de_)
    {
        for (const auto& actor: de_->actors_)
        {
            viewer_->ren_->RemoveActor( /*de_->actor_*/actor );
        }
    }

    viewer_->scheduleRedraw();
}




const IQVTKCADModel3DViewer::CADEntity &IQVTKCADModel3DViewer::HighlightItem::entity() const
{
    return entity_;
}




QModelIndex IQVTKCADModel3DViewer::HighlightItem::index() const
{
    return QModelIndex(idx2highlight_);
}
