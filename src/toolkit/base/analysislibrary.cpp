#include "base/analysislibrary.h"

#include "base/analysis.h"
#include "base/exception.h"

#include <dlfcn.h>




using namespace std;
using namespace boost;
using namespace boost::filesystem;




namespace insight {




// ====================================================================================
// ======== AnalysisLibraryLoader




AnalysisLibraryLoader::AnalysisLibraryLoader(LoadableItem it)
{
  CurrentExceptionContext ex("loading analysis libraries");

  auto paths = SharedPathList::global();
  for ( const path& p: paths )
  {
    CurrentExceptionContext ex("checking path "+p.string());

    if ( exists(p) && is_directory ( p ) )
    {
      path userconfigdir ( p );
      userconfigdir /= "modules.d";

      CurrentExceptionContext ex("checking directory "+userconfigdir.string());

      if ( exists(userconfigdir) )
      {
        if ( is_directory ( userconfigdir ) )
        {
          for ( directory_iterator itr ( userconfigdir );
                itr != directory_iterator(); ++itr )
          {
            if ( is_regular_file ( itr->status() ) )
            {
              if ( itr->path().extension() == ".module" )
              {
                CurrentExceptionContext ex("processing config file "+itr->path().string());

                std::ifstream f ( itr->path().string() );

                std::string line;
                while (getline(f, line))
                {
                  istringstream is(line);

                  std::string type;
                  is>>type;

                  if ( (type=="library") && (it==AnalysisLibrary) )
                  {
                    path location;
                    is>>location;
                    addLibrary(location);
                  }
                  else if ( (type=="guilibrary")  && (it==AnalysisVisualizationLibrary) )
                  {
                    path location;
                    is>>location;
                    addLibrary(location);
                  }
                }

              }
            }
          }
        }
      }
    }
    else
    {
      //cout<<"Not existing: "<<p<<endl;
    }
  }
}




AnalysisLibraryLoader::~AnalysisLibraryLoader()
{}




void AnalysisLibraryLoader::addLibrary(const boost::filesystem::path& location)
{
  CurrentExceptionContext ex("loading library "+location.string());

  boost::filesystem::path libFile = location;
  if (libFile.extension().empty())
  {
#ifdef WIN32
    libFile = libFile.parent_path()/("lib"+libFile.stem().string()+".dll");
#else
    libFile = libFile.parent_path()/("lib"+libFile.stem().string()+".so");
#endif
    dbg()<<libFile<<std::endl;
  }

#ifdef WIN32
  HMODULE lib = LoadLibraryA(libFile.string().c_str());
  if (!lib)
  {
    std::cerr<<"Could not load module library "<<libFile<< std::endl;
  }
#else
    void *handle = dlopen ( libFile.c_str(), RTLD_LAZY|RTLD_GLOBAL|RTLD_NODELETE );
    if ( !handle )
    {
        std::cerr<<"Could not load module library "<<libFile<<"!\nReason: " << dlerror() << std::endl;
    } else
    {
        handles_.push_back ( handle );
    }
#endif
}


AnalysisLibraryLoader AnalysisLibraryLoader::theAnalysisLibraries(AnalysisLibraryLoader::AnalysisLibrary);

AnalysisLibraryLoader& AnalysisLibraryLoader::analysisLibraries()
{
    return theAnalysisLibraries;
}



std::set<std::string> Analysis::availableAnalysisTypes(
    const std::set<std::string>& restrictToCategories )
{
    AnalysisLibraryLoader::analysisLibraries();
    std::set<std::string> al;
    for (auto &f: *insight::Analysis::factories_)
    {
        if (restrictToCategories.size())
        {
            if (restrictToCategories.count(
                    Analysis::categoryFor(f.first))<1)
                continue;
        }
        al.insert(f.first);
    }
    // std::transform(
    //             insight::Analysis::factories_->begin(),
    //             insight::Analysis::factories_->end(),
    //             std::inserter(al, al.begin()),
    //             [](const insight::Analysis::FactoryTable::value_type &f)
    //             { return f.first; }
    // );
    return al;
}



} // namespace insight
