#ifndef EDITORWITHSAVABLESTATE_H
#define EDITORWITHSAVABLESTATE_H

#include "toolkit_gui_export.h"
#include "base/rapidxml.h"

class TOOLKIT_GUI_EXPORT EditorWithSavableState
{
public:
    virtual void saveState(
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& rootNode,
        const boost::filesystem::path& parentPath ) const =0;

    virtual void restoreState(
        const rapidxml::xml_node<>& rootNode,
        const boost::filesystem::path& parentPath ) =0;

    void saveToFile(const boost::filesystem::path& file) const;
};

#endif // EDITORWITHSAVEABLESTATE_H
