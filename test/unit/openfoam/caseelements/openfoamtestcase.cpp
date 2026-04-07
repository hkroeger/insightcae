#include "openfoamtestcase.h"
#include "openfoam/ofes.h"
#include "base/boost_include.h"
#include "base/elementpath.h"
#include <exception>


OpenFOAMTestCase::OpenFOAMTestCase(const string &OFEname, CaseFeatures exclFeats)
  : OpenFOAMCase(OFEs::get(OFEname)),
    dir_(false),
    exclFeats_(exclFeats)
{}



void OpenFOAMTestCase::runTest()
{
    try
    {
        run();
    }
    catch(...)
    {
        // report contents of all files
        for (auto& d : std::vector<boost::filesystem::path>{"0", "constant", "system"})
        {
            auto dir=dir_/d;

            for (const auto& entry : boost::filesystem::recursive_directory_iterator(dir))
            {
                if (
                    !boost::filesystem::is_regular_file(entry)
                    || boost::filesystem::path_contains_file(
                        dir_/"constant"/"polyMesh",
                        entry.path())
                    ) continue;

                std::ifstream f(entry.path());
                if (!f) continue;

                std::cerr
                    << "=== " << entry.path() << " ===\n"
                    << f.rdbuf()
                    << '\n';
            }
        }

        std::rethrow_exception(std::current_exception());
    }
}

int executeTest(std::function<void(void)> testFunction)
{
  try
  {
    testFunction();
    return 0;
  }
  catch(const insight::UnsupportedFeature& ue)
  {
    cout
       <<"Skipped because feature is unsupported:"<<endl
       <<ue<<endl;
    return 0;
  }
  catch(const std::exception& e)
  {
    cout<<e.what()<<endl;
    return 1;
  }
}
