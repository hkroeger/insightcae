#ifndef EDITORWITHSAVABLESTATE_H
#define EDITORWITHSAVABLESTATE_H

#include "toolkit_gui_export.h"
#include "base/rapidxml.h"

#include "boost/signals2.hpp"
#include <boost/signals2/connection.hpp>
#include <boost/signals2/variadic_signal.hpp>

/*
 * classes are mean to be inherited parallel with other QObject-derived classes
 * => can't be derived from QObject, cannot use signals/slots
 * => use boost::signals instead
 */



class TOOLKIT_GUI_EXPORT EditorWithSavableState
{
public:

    /**
     * @brief onModification
     * should be triggered, when editor content has been modified,
     * such that saving to file would result in other content
     */
    boost::signals2::signal<void()> modificationMade;

    virtual void saveState(
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& rootNode,
        const boost::filesystem::path& parentPath ) const =0;

    virtual void restoreState(
        const rapidxml::xml_node<>& rootNode,
        const boost::filesystem::path& parentPath ) =0;

    void saveToFile(const boost::filesystem::path& file) const;
    void loadFromFile(const boost::filesystem::path& file);

};


class AutosavableEditor
: public EditorWithSavableState
{

    boost::filesystem::path currentFileName_;
    bool isModified_;

    boost::signals2::scoped_connection
        autosaveTriggerConnection_;

public:
    AutosavableEditor();

    boost::signals2::signal<void(const boost::filesystem::path&)> fileNameChanged;
    boost::signals2::signal<void()> modificationStateChanged;

    inline bool currentFileNameIsSet() const
    { return !currentFileName_.empty(); }

    inline const boost::filesystem::path& currentFileName() const
    { return currentFileName_; }

    inline bool isModified() const
    { return isModified_; }

    void load(
        const boost::filesystem::path& fromFile,
        boost::optional<boost::filesystem::path> overrideCurrentFileName
         = boost::optional<boost::filesystem::path>(),
        bool removeFromFileIfOverridden = false );
    void saveAs(const boost::filesystem::path& toFile);
    void save();

    virtual void scheduleAutosave() const =0;

    static boost::filesystem::path autosaveFileName(
        const boost::filesystem::path& p);

    void autosave() const;
};

#endif // EDITORWITHSAVEABLESTATE_H
