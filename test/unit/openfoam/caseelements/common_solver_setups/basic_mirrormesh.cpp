#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/basic/mirrormesh.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

//    // case with a reference to some extra file
//    class Case
//        : public SimpleFoamOpenFOAMCase
//    {
//    public:
//      Case(const std::string& ofe)
//        : SimpleFoamOpenFOAMCase(ofe)
//      {}

//       void createMesh() const override
//       {
//         auto res = SimpleFoamOpenFOAMCase::createDictionaries();

//         auto& cd = res->lookupDict("system/fvSchemes");
//         cd["#include"]="\""+fileName.string()+"\"";

//         return res;
//       }

//    } tc(argv[1]);

  });
}
