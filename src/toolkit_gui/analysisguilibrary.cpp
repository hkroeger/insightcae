#include "analysisguilibrary.h"

namespace insight {


AnalysisGUILibraryLoader::AnalysisGUILibraryLoader()
    : AnalysisLibraryLoader(AnalysisLibraryLoader::AnalysisVisualizationLibrary)
{}

AnalysisGUILibraryLoader AnalysisGUILibraryLoader::theGUILibraries;

AnalysisGUILibraryLoader &AnalysisGUILibraryLoader::guiLibraries()
{
    return theGUILibraries;
}


} // namespace insight
