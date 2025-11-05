#include "base/exception.h"
#include "base/filecontainer.h"
#include "base/parameterset.h"

#include <iostream>

#include "boost/filesystem/operations.hpp"
#include "test_pdl.h"

using namespace std;
using namespace insight;

int main()
{
    try
    {
        auto ps = TestPDL::defaultParameters();

        std::cout
            <<"======== CONTENT ========\n"
            <<*ps
            <<"=========================\n\n"
            <<std::endl;

        auto &et=ps->get<DoubleParameter>("run/regime/endTime");
        std::cout<<"name="<<et.name()<<std::endl;
        std::cout<<"path="<<et.path()<<std::endl;

        auto &np=ps->get<DoubleParameter>("run/initialization/preRuns/resolutions/1/nax_parameter");
        insight::assertion(
            np.name()=="nax_parameter",
            "expected another name!") ;
        insight::assertion(
            np.path()=="run/initialization/preRuns/resolutions/1/nax_parameter",
            "expected another path!") ;

        std::cout<<"path="<<np.path(true)<<std::endl;
        insight::assertion(
            np.path(true)=="run/initialization/preRuns/resolutions/default/nax_parameter",
            "expected another path with default redirection!") ;


        std::cout<<ps->getPath("absFile")<<std::endl;

        auto &afile = ps->get<PathParameter>("absFile");
        std::cout<<afile.filePath(false)<<std::endl;

        std::cout<<ps->getPath("relFile")<<std::endl;
        try
        {
            std::cout<<ps->get<PathParameter>("relFile").filePath(false)<<std::endl;
            throw insight::Exception("expected error during attempt to get full path of relative path without base directory set");
        }
        catch (const insight::UnsetBaseDirectory& ex)
        {
            // expected
        }

        // resolve relative paths
        ps->resolveRelativePaths( boost::filesystem::current_path() );


        auto rp = ps->get<PathParameter>("relFile").filePath(false);
        std::cout<<rp<<std::endl;

        insight::assertion(
            rp==boost::filesystem::current_path()/ps->getPath("relFile"),
            "unexpected path");

        return 0;
    }
    catch (std::exception& e)
    {
        cerr<<e.what()<<endl;
        return -1;
    }
}
