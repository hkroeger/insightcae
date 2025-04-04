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
: public QObject
{
    Q_OBJECT

    QAction *undoAction_, *redoAction_;
    std::stack<IQUndoRedoStackStatePtr> undoStates_, redoStates_;

protected:
    virtual void applyUndoState(const IQUndoRedoStackState& state) =0;
    virtual IQUndoRedoStackStatePtr createUndoState(const QString& description) const =0;

public:
    IQUndoRedoStack(QObject *parent=nullptr);
    void addUndoAction(QAction* act);
    void addRedoAction(QAction* act);


public Q_SLOTS:
    void storeUndoState(const QString& description);

    void undo();
    void redo();
};




#endif // IQUNDOREDOSTACK_H
