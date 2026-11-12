#ifndef INSIGHT_ASSISTANT_ISCADASSISTANTHOST_H
#define INSIGHT_ASSISTANT_ISCADASSISTANTHOST_H

#include <functional>
#include <optional>

#include <QPair>

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

    /**
     * result of an operation on the built model
     */
    struct Inspection
    {
        bool ok = false;
        QString text;
    };

    struct Vec3
    {
        double x=0, y=0, z=0;
    };

    /**
     * the symbols of the model from the last parse/rebuild
     */
    struct SymbolTable
    {
        bool ok = false;
        QString error;

        /**
         * value is empty, if not evaluated
         */
        QList<QPair<QString, std::optional<double> > > scalars;
        QList<QPair<QString, std::optional<Vec3> > > points, directions;
        QStringList datums;

        struct FeatureInfo
        {
            QString name;
            bool isComponent = false;
            bool built = false;
        };
        QList<FeatureInfo> features;
    };

    /**
     * properties of a built feature. Empty optionals: not available.
     */
    struct FeatureProperties
    {
        bool ok = false;
        QString error;

        bool isComponent = false;
        int nSolids=0, nShells=0, nFaces=0, nEdges=0, nVertices=0;
        std::optional<double> volume, surfaceArea;
        std::optional<Vec3> bbMin, bbMax;

        /**
         * center of gravity of the volume (or of the surface, if there is no volume)
         */
        std::optional<Vec3> cog;
        bool cogIsSurface = false;

        QStringList subfeatures;
        QList<QPair<QString, double> > scalars;
        QList<QPair<QString, Vec3> > points, vectors;
        QStringList datums;
    };

    /**
     * the symbols of the model as parsed/built by the last rebuild
     */
    virtual SymbolTable symbols();

    /**
     * properties of a built feature
     * @param name
     * symbol name, optionally with subfeature path (e.g. "m.instanceFinal")
     */
    virtual FeatureProperties featureProperties(const QString& name);

    /**
     * render the 3D view into an image (the user's view is restored afterwards)
     * @param direction
     * "current", "iso", "front", "back", "top", "bottom", "left" or "right"
     * @param highlight
     * feature to highlight, empty: none
     * @param png
     * receives the image (PNG)
     */
    virtual Inspection captureView(
        const QString& direction,
        const QString& highlight,
        QByteArray& png );
};


} // namespace assistant
} // namespace insight

#endif // INSIGHT_ASSISTANT_ISCADASSISTANTHOST_H
