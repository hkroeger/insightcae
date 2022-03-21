#ifndef INSIGHT_ANALYSISLIBRARY_H
#define INSIGHT_ANALYSISLIBRARY_H




#include "base/boost_include.h"
#include <vector>



namespace insight {




class AnalysisLibraryLoader
{
public:
  enum LoadableItem
  {
    AnalysisLibrary,
    AnalysisVisualizationLibrary
  };
protected:
    std::vector<void*> handles_;

public:
    AnalysisLibraryLoader(LoadableItem it = AnalysisLibrary);
    ~AnalysisLibraryLoader();

    void addLibrary(const boost::filesystem::path& lib);
};




extern AnalysisLibraryLoader analysisLibraries;




} // namespace insight

#endif // INSIGHT_ANALYSISLIBRARY_H
