#include "iqundoredostack.h"

#include "base/boost_include.h"
#include "base/exception.h"
#include "base/translations.h"



IQUndoRedoStackState::IQUndoRedoStackState(const QString &description)
    : description_(description)
{}


IQUndoRedoStackState::~IQUndoRedoStackState()
{}


const QString &IQUndoRedoStackState::description() const
{
    return description_;
}






IQUndoRedoStack::IQUndoRedoStack(QObject *parent)
    : QObject(parent)
{}




void IQUndoRedoStack::addUndoAction(QAction* act)
{
    undoAction_=act;
    connect(act, &QAction::triggered,
            this, &IQUndoRedoStack::undo);
    undoAction_->setEnabled(false);
    undoAction_->setToolTip("Nothing to be undone");
}


void IQUndoRedoStack::addRedoAction(QAction* act)
{
    redoAction_=act;
    connect(act, &QAction::triggered,
                     this, &IQUndoRedoStack::redo);
    redoAction_->setEnabled(false);
    redoAction_->setToolTip("Nothing to be redone");
}



void IQUndoRedoStack::storeUndoState(const QString &description)
{
    auto state=createUndoState(description);

    undoStates_.push(state);

    undoAction_->setEnabled(true);
    undoAction_->setToolTip(
        undoStates_.top()->description());

    redoAction_->setEnabled(false);
    redoAction_->setToolTip("Nothing to be redone");
}




void IQUndoRedoStack::undo()
{
    insight::CurrentExceptionContext ex(_("undo sketch editing step"));
    if (undoStates_.size())
    {
        auto stateToRestore = undoStates_.top();
        insight::CurrentExceptionContext ex(
            str(boost::format(_("undo step %s"))
                % stateToRestore->description().toStdString()));


        undoStates_.pop();
        redoStates_.push(createUndoState(stateToRestore->description()));

        applyUndoState(*stateToRestore);

        redoAction_->setEnabled(true);
        redoAction_->setToolTip(redoStates_.top()->description());
    }

    if (!undoStates_.size())
    {
        undoAction_->setEnabled(false);
        undoAction_->setToolTip("Nothing to be undone");
    }
    else
    {
        undoAction_->setEnabled(true);
        undoAction_->setToolTip(undoStates_.top()->description());
    }
}


void IQUndoRedoStack::redo()
{
    insight::CurrentExceptionContext ex(_("redo sketch editing step"));
    if (redoStates_.size())
    {
        auto stateToRestore = redoStates_.top();
        insight::CurrentExceptionContext ex(
            str(boost::format(_("redo step %s"))
                % stateToRestore->description().toStdString()));

        redoStates_.pop();
        undoStates_.push(createUndoState(stateToRestore->description()));

        applyUndoState(*stateToRestore);

        undoAction_->setEnabled(true);
        undoAction_->setToolTip(undoStates_.top()->description());
    }

    if (!redoStates_.size())
    {
        redoAction_->setEnabled(false);
        redoAction_->setToolTip("Nothing to be redone");
    }
    else
    {
        redoAction_->setEnabled(true);
        redoAction_->setToolTip(redoStates_.top()->description());
    }
}

