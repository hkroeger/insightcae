#ifndef SELECTIONLOGIC_H
#define SELECTIONLOGIC_H

#include "viewwidgetaction.h"
#include "boost/signals2.hpp"

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
    class SelectedEntityCompare = std::less<_SelectedEntity>
    >
class SelectionLogic
    : public Base
{

public:

    typedef _SelectedEntity SelectedEntity;
    typedef _MultiSelectionContainer MultiSelectionContainer;
    typedef std::function<bool(SelectedEntity)> SelectionFilterFunction;

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
            insight::dbg() << "selection candidate list created." << std::endl;
            insight::assertion(
                this->size()>0,
                "selection candidate list must not be empty!");

                temporaryHighlighting_ = sl_.highlightEntity( selected(), sl_.candidateColor );
        }

        ~SelectionCandidates()
        {
            insight::dbg() << "selection candidate list destroyed." << std::endl;
        }

        void cycleCandidate()
        {
            selectedCandidate_++;
            if (selectedCandidate_>=this->size())
                selectedCandidate_=0;

            temporaryHighlighting_ = boost::blank();
            temporaryHighlighting_ = sl_.highlightEntity( selected(), sl_.candidateColor );
        }

        SelectedEntity selected() const
        {
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

    std::map<SelectedEntity, HighlightingHandle,SelectedEntityCompare> highlights_;

    SelectionFilterFunction selectionFilter_;

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


    bool onLeftButtonDown(
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override
    {
        auto selectedEntities = findEntitiesUnderCursorFiltered(point);

        if (selectedEntities.size()>0)
        {
            previewHighlight_ = boost::blank();
            nextSelectionCandidates_=
                std::make_shared<SelectionCandidates>
                (*this, selectedEntities);
            return true;
        }

        return Base::onLeftButtonDown(nFlags, point);
    }


    void onMouseMove(
            Qt::MouseButtons buttons,
            const QPoint point,
            Qt::KeyboardModifiers curFlags
        ) override
    {
        if (!nextSelectionCandidates_ && doPreviewSelection_)
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
                        // nothing to do
                        return;
                    }
                }
                previewHighlight_ = std::make_pair(euc[0], highlightEntity(euc[0], candidateColor));
                newPreviewEntity(euc[0]);
                return;
            }
        }

        previewHighlight_ = boost::blank();

        Base::onMouseMove(buttons, point, curFlags);
    }

    bool onKeyPress(
        Qt::KeyboardModifiers modifiers,
        int key ) override
    {
        if (key==Qt::Key_Space)
        {
            if (nextSelectionCandidates_)
            {
                nextSelectionCandidates_->cycleCandidate();
            }
            return true;
        }
        return Base::onKeyPress(modifiers, key);
    }




    bool onLeftButtonUp(
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override
    {
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

            auto selcand = nextSelectionCandidates_->selected();
            nextSelectionCandidates_.reset();

            if (currentSelection_->count(selcand)<1)
            {

                currentSelection_->insert(selcand);
                highlights_[selcand]=highlightEntity(selcand, selectionColor);

                entitySelected(selcand); // last, because current obj might be deleted
            }
            else // remove from sel
            {
                currentSelection_->erase(selcand);
                highlights_.erase(selcand);
            }

            return true;
        }
        else // nothing was under cursor
        {
            clearSelection();
        }
        return Base::onLeftButtonUp(nFlags, point);
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
        return bool(nextSelectionCandidates_)
               && (nextSelectionCandidates_->size()>0);
    }


    SelectedEntity currentSelectionCandidate() const
    {
        return nextSelectionCandidates_->selected();
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

};


#endif // SELECTIONLOGIC_H
