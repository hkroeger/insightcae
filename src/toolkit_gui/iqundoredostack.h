#ifndef IQUNDOREDOSTACK_H
#define IQUNDOREDOSTACK_H

#include <memory>
#include <stack>

#include <QString>
#include <QAction>



class IQUndoRedoStackState
{
    QString description_;

public:
    IQUndoRedoStackState(
        const QString& description );

    virtual ~IQUndoRedoStackState();

    const QString& description() const;
};


typedef
    std::shared_ptr<IQUndoRedoStackState>
    IQUndoRedoStackStatePtr;



class IQUndoRedoStack
{

    QAction *undoAction_, *redoAction_;
    QString initialUndoActionText_, initialRedoActionText_;

    std::stack<IQUndoRedoStackStatePtr> undoStates_, redoStates_;

protected:
    virtual void applyUndoState(const IQUndoRedoStackState& state) =0;
    virtual IQUndoRedoStackStatePtr createUndoState(const QString& description) const =0;

    void setNoUndoAction();
    void setNoRedoAction();
    void setNextUndoAction();
    void setNextRedoAction();

public:
    IQUndoRedoStack();
    void addUndoAction(QAction* act);
    void addRedoAction(QAction* act);


    void storeUndoState(const QString& description);

    void undo();
    void redo();
};




#endif // IQUNDOREDOSTACK_H
