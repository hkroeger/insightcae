#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/basic/customdictentries.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    class Case : public SimpleFoamCylinderOpenFOAMCase
    {
    public:
      Case(const std::string& ofe)
        : SimpleFoamCylinderOpenFOAMCase(ofe)
      {}

      void createCase() override
      {
        SimpleFoamCylinderOpenFOAMCase::createCase();

        // needs to be added last since entries from other CEs shall be overwritten
        // (thus it needs to be added in an overwritten createCase method)
        customDictEntries::Parameters p;
        p.entries.push_back(
          customDictEntries::Parameters::entries_default_type{ "system/controlDict", "stopAt", "nextWrite" }
            );
        p.entries.push_back(
          customDictEntries::Parameters::entries_default_type{ "system/controlDict", "writeInterval", "1" }
            );
        insert(new customDictEntries(*this, p));
      }

    } tc(argv[1]);

    tc.runTest();

    insight::assertion(
          boost::filesystem::exists( tc.dir()/"1" ),
          "the modification had no effect"
    );
  });
}
