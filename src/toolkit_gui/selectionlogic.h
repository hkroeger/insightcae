#ifndef SELECTIONLOGIC_H
#define SELECTIONLOGIC_H

#include "viewwidgetaction.h"


template<
    class Base,
    class Viewer,
    class SelectedEntity,
    class SelectedEntityWrapper,
    class MultiSelectionContainer >
class SelectionLogic
    : public Base
{

public:
    class SelectionCandidates
        : public std::vector<std::weak_ptr<SelectedEntity> >
    {
        SelectionLogic& sl_;
        size_t selectedCandidate_;
        typename Viewer::HighlightingHandleSet temporaryHighlighting_;

    public:
        SelectionCandidates(
            SelectionLogic& sl,
            const std::vector<std::weak_ptr<SelectedEntity> >& cands )
            : sl_(sl),
              std::vector<std::weak_ptr<SelectedEntity> >(
                  cands.begin(), cands.end()),
              selectedCandidate_(0)
        {
            insight::assertion(
                this->size()>0,
                "selection candidate list must not be empty!");

            temporaryHighlighting_ =
                sl_.highlightEntity(selected());
        }

        ~SelectionCandidates()
        {
            sl_.unhighlightEntity(temporaryHighlighting_);
        }

        void cycleCandidate()
        {
            sl_.unhighlightEntity(temporaryHighlighting_);

            selectedCandidate_++;
            if (selectedCandidate_>=this->size())
                selectedCandidate_=0;

            temporaryHighlighting_ =
                sl_.highlightEntity(selected());
        }

        std::weak_ptr<SelectedEntity> selected() const
        {
            return this->operator[](selectedCandidate_);
        }
    };


private:
    virtual std::vector<std::weak_ptr<SelectedEntity> >
    findEntitiesUnderCursor(const QPoint& point) const =0;

    virtual typename Viewer::HighlightingHandleSet highlightEntity(
        std::weak_ptr<SelectedEntity> entity
        ) const =0;

    virtual void unhighlightEntity(
        typename Viewer::HighlightingHandleSet highlighters
        ) const =0;

protected:
    std::shared_ptr<SelectionCandidates> nextSelectionCandidates_;
    std::shared_ptr<MultiSelectionContainer> currentSelection_;

    std::function<std::shared_ptr<MultiSelectionContainer>()>
        multiSelectionContainerFactory_;

public:
    SelectionLogic(
        Viewer& viewer,
        std::function<std::shared_ptr<MultiSelectionContainer>()> mscf
        )
        : Base(viewer),
          multiSelectionContainerFactory_(mscf)
    {}




    bool onLeftButtonDown(
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override
    {
        auto selectedEntities = findEntitiesUnderCursor(point);
        if (selectedEntities.size()>0)
        {
            nextSelectionCandidates_=
                std::make_shared<SelectionCandidates>
                (*this, selectedEntities);
            return true;
        }
        return false;
    }




    bool onKeyPress(
        Qt::KeyboardModifiers modifiers,
        int key ) override
    {
        if (key==Qt::Key_Space)
        {
            if (nextSelectionCandidates_)
            {
                std::cout<<"cycle sel cand"<<std::endl;
                nextSelectionCandidates_->cycleCandidate();
            }
            return true;
        }
        return false;
    }




    bool onLeftButtonUp(
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override
    {
        if (nextSelectionCandidates_)
        {
            // apply sel candidate
            if (!currentSelection_)
            {
                currentSelection_ =
                    multiSelectionContainerFactory_();
            }

            auto selcand = nextSelectionCandidates_->selected();
            nextSelectionCandidates_.reset();

            currentSelection_->addToSelection(selcand);

            return true;
        }
        else // nothing was under cursor
        {
            std::cout<<"clear sel"<<std::endl;
            currentSelection_.reset();
        }
        return false;
    }


};

#endif // SELECTIONLOGIC_H
