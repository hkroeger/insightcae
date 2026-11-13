#ifndef ISCADHEADLESSHOST_H
#define ISCADHEADLESSHOST_H

#include <memory>
#include <atomic>

#include "assistant/iscadassistanthost.h"

namespace insight { namespace cad { class Model; } }


/**
 * assistant host without editor: the script is held in memory
 * and the model is built synchronously.
 * Used for the automated evaluation of the assistant.
 *
 * Builds and inspections run in a worker thread with a time limit:
 * OpenCASCADE operations on degenerate geometry can hang forever and
 * cannot be interrupted. A worker that exceeds the limit is abandoned
 * (it only holds its own copy of the model) and the call fails.
 */
class ISCADHeadlessHost
    : public insight::assistant::ISCADAssistantHost
{
    QString script_, fileName_;

    /**
     * model of the last successful rebuild
     */
    std::shared_ptr<insight::cad::Model> model_;

    /**
     * false, if the script was changed after the last rebuild
     */
    bool modelCurrent_ = false;

    /**
     * time limit for builds and inspections in seconds (<=0: no limit)
     */
    int timeLimit_;

    insight::assistant::ISCADAssistantHost::SymbolTable notBuilt() const;

    /**
     * the model is dropped after an abandoned inspection,
     * since the abandoned worker may still access it
     */
    void dropModelAfterTimeout(const QString& what);
    QString timeoutMessage_;

    static std::atomic<int> nAbandoned_;

public:
    ISCADHeadlessHost(const QString& script=QString(), const QString& fileName=QString(), int timeLimit=120);
    ~ISCADHeadlessHost();

    QString scriptText() const override;
    QString scriptFileName() const override;

    void beginAssistantTurn() override;
    void endAssistantTurn() override;
    void replaceText(int pos, int length, const QString& text) override;

    /**
     * builds synchronously, the callback is called before returning
     */
    void rebuild(int upToPos, RebuildCallback done) override;
    void cancelRebuild() override;

    QList<insight::assistant::CommandDoc> commandDocumentation() const override;
    QStringList librarySearchPaths() const override;

    SymbolTable symbols() override;
    FeatureProperties featureProperties(const QString& name) override;

    struct BuildResult
    {
        bool ok = false;
        QString message;
        int errorPos = -1;
    };

    /**
     * parse and build the script (up to the character position upToPos, -1: all)
     */
    BuildResult build(int upToPos=-1);

    /**
     * number of workers, which exceeded the time limit and might still be running
     * (the process should then be ended with _exit, since they might crash during static destruction)
     */
    static int nAbandonedWorkers();
};


#endif // ISCADHEADLESSHOST_H
