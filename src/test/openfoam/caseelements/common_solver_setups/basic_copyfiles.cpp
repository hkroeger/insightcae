#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/basic/copyfiles.h"

using namespace insight;

boost::filesystem::path fileName = "theFile.txt";

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");


    // case with a reference to some extra file
    class Case
        : public SimpleFoamCylinderOpenFOAMCase
    {
    public:
      Case(const std::string& ofe)
        : SimpleFoamCylinderOpenFOAMCase(ofe)
      {}

       std::shared_ptr<OFdicts> createDictionaries() const override
       {
         auto res = SimpleFoamOpenFOAMCase::createDictionaries();

         auto& cd = res->lookupDict("system/fvSchemes");
         cd["#include"]="\""+fileName.string()+"\"";

         return res;
       }

    } tc(argv[1]);




    // insert reference to some file in controlDict

    boost::filesystem::path fn =
        boost::filesystem::absolute(
          boost::filesystem::unique_path(
           boost::filesystem::temp_directory_path()/"theExternalFile-%%%%%.txt"
          )
        );
    {
      std::ofstream f(fn.string());
      f<<" /* dummy file: an error should be triggered, if it is not there */"<<std::endl;
    }

    copyFiles::Parameters p;
    p.files = {
      { make_filepath(fn), "system/"+fileName.string() }
    };
    tc.insert(new copyFiles(tc, p));

    tc.runTest();

  });
}
