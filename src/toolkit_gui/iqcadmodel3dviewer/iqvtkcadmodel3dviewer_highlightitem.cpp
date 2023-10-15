
#include "iqvtkcadmodel3dviewer.h"
#include "iqcaditemmodel.h"

#include "vtkProperty.h"


IQVTKCADModel3DViewer::ExposeItem::ExposeItem(
        std::shared_ptr<DisplayedEntity> de,
        QPersistentModelIndex idx2highlight,
        IQVTKCADModel3DViewer& viewer,
        QColor hicol
        )
    : IQVTKViewerState(viewer),
      de_(de),
      idx2highlight_(idx2highlight)
{
    for (auto& o: viewer_.displayedData_)
    {
        for (const auto& actor: o.second.actors_)
        {
            if ( auto act = vtkActor::SafeDownCast(actor) )
            {
                if (o.first == idx2highlight_)
                {
                    act->GetProperty()->SetOpacity(1.);
                    act->GetProperty()->SetColor(
                        hicol.redF(), hicol.greenF(), hicol.blueF() );
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
            if ( auto act = vtkActor::SafeDownCast(actor) )
            {
                act->GetProperty()->SetOpacity(1.);
                act->GetProperty()->SetColor(
                    hicol.redF(), hicol.greenF(), hicol.blueF() );
            }
            viewer_.renderer()->AddActor(actor);
        }
    }

    viewer_.scheduleRedraw();
}




IQVTKCADModel3DViewer::ExposeItem::~ExposeItem()
{
    for (auto& o: viewer_.displayedData_)
    {
        for (const auto& actor: o.second.actors_)
        {
            if (auto act = vtkActor::SafeDownCast(actor))
            {
                if ( o.first == idx2highlight_)
                {
                    // restore all display props
                    viewer_.resetDisplayProps(o.first);
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
            viewer_.renderer()->RemoveActor( actor );
        }
    }

    viewer_.scheduleRedraw();
}




const IQVTKCADModel3DViewer::CADEntity &IQVTKCADModel3DViewer::ExposeItem::entity() const
{
    return entity_;
}




QModelIndex IQVTKCADModel3DViewer::ExposeItem::index() const
{
    return QModelIndex(idx2highlight_);
}





IQVTKCADModel3DViewer::SilhouetteHighlighter::SilhouetteHighlighter(
    IQVTKCADModel3DViewer& viewer,
    vtkPolyDataMapper* mapperToHighlight,
    QColor hicol )
    : IQVTKViewerState(viewer)
{
    insight::dbg()<<"SilhouetteHighlighter created"<<std::endl;

    auto* id = mapperToHighlight->GetInput();
    if (id->GetNumberOfCells()>0)
    {
        silhouette_ = vtkSmartPointer<vtkPolyDataSilhouette>::New();
        silhouette_->SetCamera(viewer_.renderer()->GetActiveCamera());

        // Create mapper and actor for silhouette
        auto silhouetteMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        silhouetteMapper->SetInputConnection(silhouette_->GetOutputPort());

        silhouetteActor_ = vtkSmartPointer<vtkActor>::New();
        silhouetteActor_->SetMapper(silhouetteMapper);
        silhouetteActor_->GetProperty()->SetColor(
            hicol.redF(), hicol.greenF(), hicol.blueF() );
        silhouetteActor_->GetProperty()->SetLineWidth(2);

        silhouette_->SetInputData(id);
        viewer_.renderer()->AddActor(silhouetteActor_);
        viewer_.actorsExcludedFromPicking_.insert(silhouetteActor_);
        viewer_.scheduleRedraw();
    }
    else if (id->GetNumberOfLines()>0)
    {
    }
}




IQVTKCADModel3DViewer::SilhouetteHighlighter::~SilhouetteHighlighter()
{
    viewer_.renderer()->RemoveActor(silhouetteActor_);
    viewer_.actorsExcludedFromPicking_.erase(silhouetteActor_);
    viewer_.scheduleRedraw();

    insight::dbg()<<"SilhouetteHighlighter removed"<<std::endl;
}


IQVTKCADModel3DViewer::LinewidthHighlighter::LinewidthHighlighter(
    IQVTKCADModel3DViewer& viewer,
    vtkActor* actorToHighlight,
    QColor hicol )
    : IQVTKViewerState(viewer), actor_(actorToHighlight)
{

    oldLineWidth_=actor_->GetProperty()->GetLineWidth();
    actor_->GetProperty()->GetColor(oldColor_);

    actor_->GetProperty()->SetLineWidth(oldLineWidth_+2);
    actor_->GetProperty()->SetColor(
        hicol.redF(), hicol.greenF(), hicol.blueF() );

    insight::dbg()<<"LinewidthHighlighter created lw="<<(oldLineWidth_+2)<<" for actor "<<actor_<<std::endl;

    viewer_.scheduleRedraw();
}

IQVTKCADModel3DViewer::LinewidthHighlighter::~LinewidthHighlighter()
{
    actor_->GetProperty()->SetLineWidth(oldLineWidth_);
    actor_->GetProperty()->SetColor(oldColor_);

    viewer_.scheduleRedraw();

    insight::dbg()<<"LinewidthHighlighter removed, restore lw="<<oldLineWidth_<<" for actor "<<actor_<<std::endl;
}



IQVTKCADModel3DViewer::PointSizeHighlighter::PointSizeHighlighter(
    IQVTKCADModel3DViewer& viewer,
    vtkActor* actorToHighlight,
    QColor hicol )
    : IQVTKViewerState(viewer), actor_(actorToHighlight)
{
    insight::dbg()<<"PointSizeHighlighter created"<<std::endl;

    oldPointSize_=actor_->GetProperty()->GetPointSize();
    actor_->GetProperty()->GetColor(oldColor_);

    actor_->GetProperty()->SetPointSize(oldPointSize_+2);
    actor_->GetProperty()->SetColor(
        hicol.redF(), hicol.greenF(), hicol.blueF());

    viewer_.scheduleRedraw();
}

IQVTKCADModel3DViewer::PointSizeHighlighter::~PointSizeHighlighter()
{
    actor_->GetProperty()->SetPointSize(oldPointSize_);
    actor_->GetProperty()->SetColor(oldColor_);
    viewer_.scheduleRedraw();

    insight::dbg()<<"PointSizeHighlighter removed"<<std::endl;
}
