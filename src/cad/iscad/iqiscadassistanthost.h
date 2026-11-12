#ifndef IQISCADASSISTANTHOST_H
#define IQISCADASSISTANTHOST_H

#include <QObject>
#include <QPointer>
#include <QTimer>

#include "assistant/iscadassistanthost.h"

class IQISCADModelScriptEdit;


/**
 * gives the chat assistant access to the script editor of a model window
 */
class IQISCADAssistantHost
    : public QObject,
      public insight::assistant::ISCADAssistantHost
{
    Q_OBJECT

    QPointer<IQISCADModelScriptEdit> edit_;

    /**
     * the first edit of an assistant turn opens a new undo step,
     * the following ones are joined to it
     */
    bool firstEditInTurn_ = true;

    // ===== rebuild state
    RebuildCallback rebuildDone_;
    std::string rebuildScript_;
    bool rebuildFailed_ = false;
    QString rebuildError_;
    int rebuildErrorPos_ = -1;
    QTimer* launchFailureCheck_;

    void onScriptError(long failpos, const QString& errorMsg, int task);
    void onJobEnded(int task);
    void completeRebuild();

public:
    IQISCADAssistantHost(IQISCADModelScriptEdit* edit, QObject* parent=nullptr);

    QString scriptText() const override;
    QString scriptFileName() const override;

    void beginAssistantTurn() override;
    void endAssistantTurn() override;

    void replaceText(int pos, int length, const QString& text) override;

    void rebuild(int upToPos, RebuildCallback done) override;
    void cancelRebuild() override;

    QList<insight::assistant::CommandDoc> commandDocumentation() const override;
    QStringList librarySearchPaths() const override;

    enum ReferenceLevel { Full, Basic, None };

    /**
     * the ISCAD language documentation, if it was compiled into the program.
     * Full: language and command reference, Basic: language only
     */
    static QString languageReference(ReferenceLevel level = Full);
};

#endif // IQISCADASSISTANTHOST_H
