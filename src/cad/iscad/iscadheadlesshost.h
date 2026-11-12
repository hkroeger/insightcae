#ifndef ISCADHEADLESSHOST_H
#define ISCADHEADLESSHOST_H

#include <memory>

#include "assistant/iscadassistanthost.h"

namespace insight { namespace cad { class Model; } }


/**
 * assistant host without editor: the script is held in memory
 * and the model is built synchronously in the calling thread.
 * Used for the automated evaluation of the assistant.
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

    insight::assistant::ISCADAssistantHost::SymbolTable notBuilt() const;

public:
    ISCADHeadlessHost(const QString& script=QString(), const QString& fileName=QString());
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
};


#endif // ISCADHEADLESSHOST_H
