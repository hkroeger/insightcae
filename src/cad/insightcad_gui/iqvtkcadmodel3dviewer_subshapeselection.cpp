
#include "iqvtkcadmodel3dviewer.h"
#include "cadfeature.h"


IQVTKCADModel3DViewer::SubshapeSelection::SubshapeSelection(
        IQVTKCADModel3DViewer& viewer )
    : viewer_(viewer)
{}




IQVTKCADModel3DViewer::SubshapeSelection::~SubshapeSelection()
{
    for (auto& e: *this)
    {
        viewer_.ren_->RemoveActor(e.first);
    }
    for (const auto& act: temporarilyHiddenActors_)
    {
        act->SetVisibility(true);
    }
    if (temporarilyHiddenActors_.size()) viewer_.ren_->Render();

}




void IQVTKCADModel3DViewer::SubshapeSelection::add(
        insight::cad::FeaturePtr feat,
        insight::cad::EntityType subshapeType )
{
    if (const auto *disp = viewer_.findDisplayedItem(feat))
    {
        if (const auto* featPtr =
                boost::get<insight::cad::FeaturePtr>(&disp->ce_))
        {
            auto feat = *featPtr;
            if (subshapeType==insight::cad::Vertex)
            {
                auto vIDs = feat->allVerticesSet();
                for (const auto& vID: vIDs)
                {
                    SubshapeData sd{feat, subshapeType, vID};
                    auto actor = viewer_.createSubshapeActor(sd);
                    (*this)[actor]=sd;
                    viewer_.ren_->AddActor(actor);
                }
            }
            else if (subshapeType==insight::cad::Edge)
            {
                auto eIDs = feat->allEdgesSet();
                for (const auto& eID: eIDs)
                {
                    SubshapeData sd{feat, subshapeType, eID};
                    auto actor = viewer_.createSubshapeActor(sd);
                    (*this)[actor]=sd;
                    viewer_.ren_->AddActor(actor);
                }
            }
            else if (subshapeType==insight::cad::Face)
            {
                if (auto *de = viewer_.findDisplayedItem(feat))
                {
                    for (const auto& act: de->actors_)
                    {
                        if (act->GetVisibility())
                        {
                            temporarilyHiddenActors_.insert(act);
                            act->SetVisibility(false);
                        }
                    }
                }

                if (temporarilyHiddenActors_.size()) viewer_.ren_->Render();

                auto fIDs = feat->allFacesSet();
                for (const auto& fID: fIDs)
                {
                    SubshapeData sd{feat, subshapeType, fID};
                    auto actor = viewer_.createSubshapeActor(sd);
                    (*this)[actor]=sd;
                    viewer_.ren_->AddActor(actor);
                }
            }
            else if (subshapeType==insight::cad::Solid)
            {
                if (auto *de = viewer_.findDisplayedItem(feat))
                {
                    for (const auto& act: de->actors_)
                    {
                        if (act->GetVisibility())
                        {
                            temporarilyHiddenActors_.insert(act);
                            act->SetVisibility(false);
                        }
                    }
                }

                if (temporarilyHiddenActors_.size()) viewer_.ren_->Render();

                auto sIDs = feat->allSolidsSet();
                for (const auto& sID: sIDs)
                {
                    SubshapeData sd{feat, subshapeType, sID};
                    auto actor = viewer_.createSubshapeActor(sd);
                    (*this)[actor]=sd;
                    viewer_.ren_->AddActor(actor);
                }
            }
        }
    }
}

