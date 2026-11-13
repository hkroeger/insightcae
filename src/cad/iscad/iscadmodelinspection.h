#ifndef ISCADMODELINSPECTION_H
#define ISCADMODELINSPECTION_H

#include <memory>

#include "assistant/iscadassistanthost.h"
#include "assistant/referencedocs.h"

namespace insight { namespace cad {
class Feature;
class Model;
} }


/**
 * access to the built ISCAD model for the chat assistant
 * (shared by the editor and the headless evaluation host)
 */
namespace ISCADModelInspection
{

/**
 * the feature for "name" or "name.sub.sub".
 * Returns null and sets error, if it does not exist or was not built.
 * Unbuilt features are never accessed (that would trigger a build).
 */
std::shared_ptr<insight::cad::Feature> builtFeature(
    const insight::cad::Model& model,
    const QString& name,
    QString& error );

insight::assistant::ISCADAssistantHost::SymbolTable symbols(
    const insight::cad::Model& model );

insight::assistant::ISCADAssistantHost::FeatureProperties featureProperties(
    const insight::cad::Model& model,
    const QString& name );

QList<insight::assistant::CommandDoc> commandDocumentation();

QStringList librarySearchPaths();

enum ReferenceLevel { FullReference, BasicReference, NoReference, OnDemandReference };

/**
 * the ISCAD language documentation, if it was compiled into the program
 * (resource :/iscad_assistant/).
 * Full: language and command reference, Basic: language only,
 * OnDemand: the essentials and a table of contents (the rest via the tool read_reference)
 */
QString languageReference(ReferenceLevel level);

/**
 * the sections of the language and command reference for the tool read_reference
 * (empty, if the documentation was not compiled in)
 */
std::shared_ptr<const insight::assistant::ReferenceIndex> referenceIndex();

}

#endif // ISCADMODELINSPECTION_H
