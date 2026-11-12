#ifndef INSIGHT_ASSISTANT_ISCADASSISTANTHOST_H
#define INSIGHT_ASSISTANT_ISCADASSISTANTHOST_H

#include <functional>

#include <QString>
#include <QStringList>
#include <QList>

namespace insight {
namespace assistant {


struct CommandDoc
{
    QString command;
    QString signature;
    QString documentation;
};


/**
 * the access of the assistant to the model editor
 */
class ISCADAssistantHost
{
public:
    virtual ~ISCADAssistantHost() = default;

    virtual QString scriptText() const =0;

    /**
     * file name of the script, empty if not yet saved
     */
    virtual QString scriptFileName() const =0;

    /**
     * the assistant starts working on a request:
     * all following edits form a single undo step,
     * user edits are blocked until endAssistantTurn()
     */
    virtual void beginAssistantTurn() =0;
    virtual void endAssistantTurn() =0;

    /**
     * replace the characters [pos, pos+length) of the script by text
     */
    virtual void replaceText(int pos, int length, const QString& text) =0;

    /**
     * @param ok
     * true, if the model was built without error
     * @param message
     * error message (or other information)
     * @param errorPos
     * character position of the error in the script, -1 if unknown
     */
    typedef std::function<void(bool ok, const QString& message, int errorPos)> RebuildCallback;

    /**
     * rebuild the model (without executing postprocessing actions, which write files).
     * The callback is called exactly once, unless cancelRebuild() is called before.
     * @param upToPos
     * build only the script up to this character position, -1: everything
     */
    virtual void rebuild(int upToPos, RebuildCallback done) =0;
    virtual void cancelRebuild() =0;

    /**
     * documentation of all feature commands
     */
    virtual QList<CommandDoc> commandDocumentation() const =0;

    /**
     * directories, which contain library models (*.iscad)
     */
    virtual QStringList librarySearchPaths() const =0;
};


} // namespace assistant
} // namespace insight

#endif // INSIGHT_ASSISTANT_ISCADASSISTANTHOST_H
