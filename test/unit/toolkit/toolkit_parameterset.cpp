#include "base/parameterset.h"

#include <iostream>

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



        return 0;
    }
    catch (std::exception& e)
    {
        cerr<<e.what()<<endl;
        return -1;
    }
}
