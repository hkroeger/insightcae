#include "editorwithsavablestate.h"
#include "base/rapidxml.h"

void EditorWithSavableState::saveToFile(
    const boost::filesystem::path &file ) const
{
    insight::XMLDocument doc;
    saveState(doc, *doc.rootNode, file.parent_path());
    doc.saveToFile(file);
}
