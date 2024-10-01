#include "base/boost_include.h"

#include "base/exception.h"
#include "base/remoteserverlist.h"


using namespace std;
using namespace insight;
using namespace boost;

int main()
{
    try
    {
        // request server from pool
        auto srv2000 = insight::remoteServers.requestUnoccupiedServer(2000);
        insight::assertion(
            !srv2000, "expected not to be able to find a server with 2000 procs");

        auto srv20 = insight::remoteServers.requestUnoccupiedServer(20);
        std::cout<<"srv20="<<*srv20<<endl;

        auto srv100 = insight::remoteServers.requestUnoccupiedServer(100);
        std::cout<<"srv100="<<*srv100<<endl;

    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
        return -1;
    }
}
