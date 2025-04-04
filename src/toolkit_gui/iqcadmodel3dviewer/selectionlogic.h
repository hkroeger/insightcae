#ifndef SELECTIONLOGIC_H
#define SELECTIONLOGIC_H

#include "viewwidgetaction.h"
#include "boost/signals2.hpp"
#include <qnamespace.h>

template<class T>
std::ostream&
operator<<(std::ostream& os, const std::weak_ptr<T>& wp)
{
    os << wp.lock();
    return os;
}


template<
    class Base,
    class Viewer,
    class _SelectedEntity,
    class HighlightingHandle, // highlighting in viewer ends, if such object is disposed
    class _MultiSelectionContainer = std::set<_SelectedEntity>,
    class _SelectedEntityCompare = std::less<_SelectedEntity>
    >
class SelectionLogic
    : public Base
{

public:

    typedef _SelectedEntity SelectedEntity;
    typedef _MultiSelectionContainer MultiSelectionContainer;
    typedef std::function<bool(SelectedEntity)> SelectionFilterFunction;
    typedef _SelectedEntityCompare SelectedEntityCompare;

    boost::signals2::signal<void(SelectedEntity)> entitySelected, newPreviewEntity;

    QColor candidateColor = QColorConstants::Blue;
    QColor selectionColor = QColorConstants::Red;


    class SelectionCandidates
        : public std::vector<SelectedEntity>
    {
        SelectionLogic& sl_;
        size_t selectedCandidate_;
        boost::variant<HighlightingHandle,boost::blank> temporaryHighlighting_;

    public:
        SelectionCandidates(
            SelectionLogic& sl,
            const std::vector<SelectedEntity>& cands
            )
            : sl_(sl),
              std::vector<SelectedEntity>(
                  cands.begin(), cands.end()),
              selectedCandidate_(0)
        {
            insight::dbg(2) << "selection candidate list created." << std::endl;
            insight::assertion(
                this->size()>0,
                "selection candidate list must not be empty!");

            temporaryHighlighting_ = sl_.highlightEntity( selected(true), sl_.candidateColor );
        }

        ~SelectionCandidates()
        {
            insight::dbg(2) << "selection candidate list destroyed." << std::endl;
        }

        void cycleCandidate()
        {
            selectedCandidate_++;
            if (selectedCandidate_>=this->size())
                selectedCandidate_=0;

            temporaryHighlighting_ = sl_.highlightEntity( selected(true), sl_.candidateColor );
        }

        SelectedEntity selected(bool clearTempHighlighting) const
        {
            if (clearTempHighlighting)
            {
                const_cast<SelectionCandidates*>(this)
                    ->temporaryHighlighting_ = boost::blank();
            }
            return this->operator[](selectedCandidate_);
        }
    };


private:
    virtual std::vector<SelectedEntity>
    findEntitiesUnderCursor(const QPoint& point) const =0;

    std::vector<SelectedEntity>
    findEntitiesUnderCursorFiltered(const QPoint& point)
    {
        auto se = findEntitiesUnderCursor(point);

        // remove already selected entities
        if (currentSelection_)
        {
            auto seRaw(se);
            se.clear();
            std::copy_if(
                seRaw.begin(), seRaw.end(),
                std::back_inserter(se),
                [this](SelectedEntity e)
                {
                    return (currentSelection_->count(e)<1);
                }
            );
        }

        // apply filter
        if (selectionFilter_)
        {
            auto seRaw(se);
            se.clear();
            std::copy_if(
                seRaw.begin(), seRaw.end(),
                std::back_inserter(se),
                selectionFilter_ );
        }

        return se;
    }


    virtual HighlightingHandle highlightEntity(
        SelectedEntity entity,
        QColor hilightColor
        ) const =0;


    std::shared_ptr<SelectionCandidates> nextSelectionCandidates_;
    std::shared_ptr<MultiSelectionContainer> currentSelection_;

    std::function<std::shared_ptr<MultiSelectionContainer>()>
        multiSelectionContainerFactory_;

    typedef std::pair<SelectedEntity, HighlightingHandle> PreviewHighlight;
    bool doPreviewSelection_;
    boost::variant<boost::blank, PreviewHighlight> previewHighlight_;


    SelectionFilterFunction selectionFilter_;

protected:
    std::map<SelectedEntity, HighlightingHandle, SelectedEntityCompare> highlights_;

public:
    template<class ...Args>
    SelectionLogic(
        std::function<std::shared_ptr<MultiSelectionContainer>()> mscf,
        Args&&... addArgs )
        : Base(std::forward<Args>(addArgs)...),
          multiSelectionContainerFactory_(mscf),
          doPreviewSelection_(false)
    {}

    void setSelectionFilter(SelectionFilterFunction sff)
    {
        selectionFilter_=sff;
    }

    ~SelectionLogic()
    {
        clearSelection();
    }

    void toggleHoveringSelectionPreview(bool doPreviewSelection)
    {
        if (doPreviewSelection_ && !doPreviewSelection)
        {
            previewHighlight_=boost::blank();
        }
        doPreviewSelection_=doPreviewSelection;
    }

    bool hoveringSelectionPreview() const
    {
        return doPreviewSelection_;
    }

    template<class Container>
    void updateSelectionCandidates(const Container& selectedEntities)
    {
        previewHighlight_ = boost::blank();
        nextSelectionCandidates_=
            std::make_shared<SelectionCandidates>
            (*this, selectedEntities);
    }

    void updateSelectionCandidates(const QPoint& point)
    {
        if (!nextSelectionCandidates_ )
        {
            auto selectedEntities = findEntitiesUnderCursorFiltered(point);

            if (selectedEntities.size()>0)
            {
                updateSelectionCandidates(selectedEntities);

                this->userPrompt(
                    QString(
                        "There are %1 entities at picked location."
                        " Hold mouse button and press <Space> to cycle selection." )
                    .arg(nextSelectionCandidates_->size() ) );
            }
        }
    }

    // bool onMouseClick(
    //     typename Base::MouseButton btn,
    //     Qt::KeyboardModifiers nFlags,
    //     const QPoint point ) override
    // {
    //     if (!this->hasChildReceivers() || !Base::onMouseClick(btn, nFlags, point))
    //     {
    //         if (!nextSelectionCandidates_ )
    //         {
    //             auto selectedEntities = findEntitiesUnderCursorFiltered(point);

    //             if (selectedEntities.size()>0)
    //             {
    //                 previewHighlight_ = boost::blank();
    //                 nextSelectionCandidates_=
    //                     std::make_shared<SelectionCandidates>
    //                     (*this, selectedEntities);

    //                 this->userPrompt(
    //                     QString(
    //                         "There are %1 entities at picked location."
    //                         " Hold mouse button and press <Space> to cycle selection." )
    //                     .arg(nextSelectionCandidates_->size() ) );

    //                 return true;
    //             }
    //         }
    //     }
    //     return false;
    // }



    bool onKeyPress(
        Qt::KeyboardModifiers modifiers,
        int key ) override
    {
        if (!this->hasChildReceivers() /*|| !Base::onKeyPress(modifiers, key)*/)
        {
            if ( (key==Qt::Key_Space)
                || (key==Qt::Key_Tab) )
            {
                if (!nextSelectionCandidates_ && this->hasLastMouseLocation() )
                {
                    auto selectedEntities =
                        findEntitiesUnderCursorFiltered(
                            this->lastMouseLocation()
                        );

                    if (selectedEntities.size()>0)
                    {
                        previewHighlight_ = boost::blank();
                        nextSelectionCandidates_=
                            std::make_shared<SelectionCandidates>
                            (*this, selectedEntities);

                        this->userPrompt(
                            QString(
                                "There are %1 entities at the current location."
                                " Don't move the mouse and press <Space> or <Tab> to cycle selection." )
                                .arg(nextSelectionCandidates_->size() ) );

                        return true;
                    }
                }
                else
                {
                    nextSelectionCandidates_->cycleCandidate();
                    return true;
                }
            }
        }
        return Base::onKeyPress(modifiers, key);
    }




    bool onMouseClick(
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override
    {
        if (!this->hasChildReceivers())
        {
            if (btn==Qt::LeftButton)
            {
                if (!nextSelectionCandidates_)
                {
                    updateSelectionCandidates(point);
                }
                if (nextSelectionCandidates_)
                {
                    if (!(nFlags&Qt::ShiftModifier))
                    {
                        clearSelection();
                    }

                    // apply sel candidate
                    if (!currentSelection_)
                    {
                        currentSelection_ =
                            multiSelectionContainerFactory_();
                    }

                    auto selcand = nextSelectionCandidates_->selected(true);
                    nextSelectionCandidates_.reset();

                    if (currentSelection_->count(selcand)<1)
                    {
                        currentSelection_->insert(selcand);
                        this->userPrompt(
                            QString("Added to selection. Now %1 entities selected.")
                                .arg(currentSelection_->size()));

                        if (highlights_.count(selcand)<1)
                            highlights_[selcand]=highlightEntity(selcand, selectionColor);

                        this->updateLastMouseLocation(point);
                        entitySelected(selcand); // last, because current obj might be deleted
                    }
                    else // remove from sel
                    {
                        currentSelection_->erase(selcand);
                        this->userPrompt(
                            QString("Removed from selection. Now %1 entities selected.")
                                .arg(currentSelection_->size()));
                        highlights_.erase(selcand);
                    }

                    return true;
                }
                else // nothing was under cursor
                {
                    if (currentSelection_ && currentSelection_->size()>0)
                    {
                        clearSelection();
                        this->userPrompt("Nothing picked. Selection cleared.");
                        return true;
                    }
                }
            }
        }
        return Base::onMouseClick(btn, nFlags, point);
    }


    bool updatePreview(const QPoint& point)
    {
        auto euc = findEntitiesUnderCursorFiltered(point);
        if (euc.size()>0)
        {
            if (auto *ph = boost::get<PreviewHighlight>(&previewHighlight_))
            {
                if ( !SelectedEntityCompare()(ph->first, euc[0])
                    &&
                    !SelectedEntityCompare()(euc[0], ph->first) )
                {
                    // already in preview highlight, nothing to do
                    return false;
                }
            }

            previewHighlight_ = boost::blank();
            previewHighlight_ = std::make_pair(euc[0], highlightEntity(euc[0], candidateColor));

            newPreviewEntity(euc[0]);

            return true;
        }

        // else
        previewHighlight_ = boost::blank();
        return false;
    }



    bool onMouseMove(
            const QPoint point,
            Qt::KeyboardModifiers curFlags
        ) override
    {
        if (!this->hasChildReceivers())
        {
            if ( !nextSelectionCandidates_
                 && doPreviewSelection_ )
            {
                updatePreview(point);
            }
        }

        return Base::onMouseMove(point, curFlags);
    }




    void onMouseLeavesViewer() override
    {
        previewHighlight_ = boost::blank();
    }


    bool hasPreviewedItem() const
    {
        if (auto *ph = boost::get<PreviewHighlight>(&previewHighlight_))
        {
            return true;
        }
        return false;
    }


    SelectedEntity previewedItem() const
    {
        auto ph = boost::get<PreviewHighlight>(previewHighlight_);
        return ph.first;
    }


    bool hasSelectionCandidate() const
    {
        insight::dbg(3)
            <<"nextSelectionCandidates:"
            <<(bool(nextSelectionCandidates_)?nextSelectionCandidates_->size():0)
            <<std::endl;

        return bool(nextSelectionCandidates_)
               && (nextSelectionCandidates_->size()>0);
    }


    SelectedEntity currentSelectionCandidate() const
    {
        return nextSelectionCandidates_->selected(false);
    }

    bool somethingSelected() const
    {
        return bool(currentSelection_)
               && (currentSelection_->size()>0);
    }

    const MultiSelectionContainer& currentSelection() const
    {
        return *currentSelection_;
    }

    void clearSelection()
    {
        currentSelection_.reset();
        highlights_.clear();
    }

    void externallySelect(SelectedEntity entity)
    {
        if (!currentSelection_)
        {
            currentSelection_ =
                multiSelectionContainerFactory_();
        }

        currentSelection_->insert(entity);
        this->userPrompt(
            QString("Added to selection. Now %1 entities selected.")
                .arg(currentSelection_->size()));

        if (highlights_.count(entity)<1)
            highlights_[entity]=highlightEntity(entity, selectionColor);

        entitySelected(entity);
    }

    void externallyUnselect(SelectedEntity entity)
    {
        if (currentSelection_)
        {
            if (currentSelection_->count(entity)>0)
            {
                currentSelection_->erase(entity);
                this->userPrompt(
                    QString("Removed from selection. Now %1 entities selected.")
                        .arg(currentSelection_->size()));
                highlights_.erase(entity);
            }
        }
    }

    template<class Container>
    void setSelectionTo(const Container& selectedEntities)
    {
        std::set<SelectedEntity, SelectedEntityCompare> tbr;
        if (currentSelection_)
        {
            for (auto& cs: *currentSelection_)
            {
                tbr.insert(cs);
            }
        }

        for (auto& selcand: selectedEntities)
        {
            externallySelect(selcand);
            tbr.erase(selcand);
        }

        for (auto& e: tbr)
        {
            externallyUnselect(e);
        }
    }

};


#endif // SELECTIONLOGIC_H
