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






void IQUndoRedoStack::setNoUndoAction()
{
    undoAction_->setText(initialUndoActionText_);
    undoAction_->setEnabled(false);
    undoAction_->setToolTip(_("Nothing to be undone"));
}




void IQUndoRedoStack::setNoRedoAction()
{
    redoAction_->setText(initialRedoActionText_);
    redoAction_->setEnabled(false);
    redoAction_->setToolTip(_("Nothing to be redone"));
}




void IQUndoRedoStack::setNextUndoAction()
{
    undoAction_->setText(
        initialUndoActionText_
        + " ("
        + undoStates_.top()->description()
        + ")" );
    undoAction_->setEnabled(true);
    undoAction_->setToolTip(
        undoStates_.top()->description() );
}




void IQUndoRedoStack::setNextRedoAction()
{
    redoAction_->setText(
        initialRedoActionText_
        + " ("
        + redoStates_.top()->description()
        + ")" );

    redoAction_->setEnabled(true);
    redoAction_->setToolTip(
        redoStates_.top()->description() );
}




IQUndoRedoStack::IQUndoRedoStack()
{}




void IQUndoRedoStack::addUndoAction(QAction* act)
{
    undoAction_=act;
    initialUndoActionText_=act->text();

    QObject::connect(act, &QAction::triggered, act,
            std::bind(&IQUndoRedoStack::undo, this) );

    setNoUndoAction();
}


void IQUndoRedoStack::addRedoAction(QAction* act)
{
    redoAction_=act;
    initialRedoActionText_=act->text();

    QObject::connect(act, &QAction::triggered, act,
                     std::bind(&IQUndoRedoStack::redo, this) );

    setNoRedoAction();
}



void IQUndoRedoStack::storeUndoState(const QString &description)
{
    insight::CurrentExceptionContext ex(
        "store undo state " + description.toStdString());

    auto state=createUndoState(description);

    undoStates_.push(state);

    setNextUndoAction();
    setNoRedoAction();
}




void IQUndoRedoStack::undo()
{
    insight::CurrentExceptionContext ex(_("perform undo step"));
    if (undoStates_.size())
    {
        auto stateToRestore = undoStates_.top();
        insight::CurrentExceptionContext ex(
            str(boost::format(_("undo step %s"))
                % stateToRestore->description().toStdString()));


        undoStates_.pop();
        redoStates_.push(createUndoState(stateToRestore->description()));

        applyUndoState(*stateToRestore);

        setNextRedoAction();
    }

    if (!undoStates_.size())
    {
        setNoUndoAction();
    }
    else
    {
        setNextUndoAction();
    }
}




void IQUndoRedoStack::redo()
{
    insight::CurrentExceptionContext ex(_("perform redo step"));
    if (redoStates_.size())
    {
        auto stateToRestore = redoStates_.top();
        insight::CurrentExceptionContext ex(
            str(boost::format(_("redo step %s"))
                % stateToRestore->description().toStdString()));

        redoStates_.pop();
        undoStates_.push(createUndoState(stateToRestore->description()));

        applyUndoState(*stateToRestore);

        setNextUndoAction();
    }

    if (!redoStates_.size())
    {
        setNoRedoAction();
    }
    else
    {
        setNextRedoAction();
    }
}

