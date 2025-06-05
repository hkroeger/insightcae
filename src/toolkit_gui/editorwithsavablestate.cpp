#include "editorwithsavablestate.h"
#include "base/exception.h"
#include "base/rapidxml.h"
#include <boost/filesystem/operations.hpp>

void EditorWithSavableState::saveToFile(
    const boost::filesystem::path &file ) const
{
    insight::XMLDocument doc;
    saveState(doc, *doc.rootNode, file.parent_path());
    doc.saveToFile(file);
}

void EditorWithSavableState::loadFromFile(const boost::filesystem::path &file)
{
    insight::XMLDocument doc(file);
    restoreState(*doc.rootNode, file.parent_path());
}



AutosavableEditor::AutosavableEditor()
    : isModified_(false)
{
    autosaveTriggerConnection_=
        modificationMade.connect(
            [this]()
            {
                isModified_=true;
                modificationStateChanged();
                scheduleAutosave();
            }
    );
}


void AutosavableEditor::load(
    const boost::filesystem::path& fromFile,
    boost::optional<boost::filesystem::path> overrideCurrentFileName,
    bool removeFromFileIfOverridden )
{
    currentFileName_=
        overrideCurrentFileName ? *overrideCurrentFileName : fromFile;
    fileNameChanged(currentFileName_);

    {
        boost::signals2::shared_connection_block blocker(
            autosaveTriggerConnection_ );

        loadFromFile(fromFile);
    }

    if (overrideCurrentFileName
        && removeFromFileIfOverridden)
    {
        boost::filesystem::remove(fromFile);
    }
    isModified_=false;
    modificationStateChanged();
}

void AutosavableEditor::saveAs(const boost::filesystem::path& toFile)
{
    if (currentFileName_!=toFile)
    {
        currentFileName_=toFile;
        fileNameChanged(currentFileName_);
    }
    save();
}

void AutosavableEditor::save()
{
    insight::assertion(
        currentFileNameIsSet(),
        "no file name has been set");
    saveToFile(currentFileName_);

    auto asfn=autosaveFileName(currentFileName_);
    if (boost::filesystem::exists(asfn))
    {
        boost::filesystem::remove(asfn);
    }

    isModified_=false;
    modificationStateChanged();
}

boost::filesystem::path
AutosavableEditor::autosaveFileName(
    const boost::filesystem::path& p)
{
    auto cfn=p;
    return
        cfn.parent_path() /
        ( "."
         + cfn.filename().stem().string()
         + ".autosave"
         + cfn.extension().string() );
}


void AutosavableEditor::autosave() const
{
    boost::filesystem::path savefn;
    if (currentFileNameIsSet())
    {
        savefn=autosaveFileName(
            currentFileName() );
    }
    if (!savefn.empty())
    {
        insight::dbg()
            << "autosave into "
            << savefn.string()
            << std::endl;

        saveToFile(savefn);
    }
}
