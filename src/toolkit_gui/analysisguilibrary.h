#ifndef INSIGHT_ANALYSISGUILIBRARY_H
#define INSIGHT_ANALYSISGUILIBRARY_H

#include "base/analysislibrary.h"

namespace insight {

class AnalysisGUILibraryLoader
 : public AnalysisLibraryLoader
{
    AnalysisGUILibraryLoader();
    static AnalysisGUILibraryLoader theGUILibraries;

public:
    static AnalysisGUILibraryLoader& guiLibraries();
};


} // namespace insight

#endif // INSIGHT_ANALYSISGUILIBRARY_H
