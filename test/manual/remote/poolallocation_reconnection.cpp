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
        auto scfg = insight::remoteServers.requestUnoccupiedServer(1, "docker_test_pool");
        insight::assertion(
            bool(scfg), "expected to find a server!");

        insight::assertion(
          !scfg->isRunning(),
            "expected server not to be running" );

        {
            // launch
            auto srv = scfg->instance();
        }

        insight::assertion(
            scfg->isRunning(),
            "expected server to be still running" );

        {
            // grab again
            auto srv = scfg->getInstanceIfRunning();
            insight::assertion(
                bool(srv), "expected to get an instance!");

            // stop and deallocte
            srv->destroyIfPossible();
        }

        insight::assertion(
            !scfg->getInstanceIfRunning(), "expected not to get a server instance!");

        insight::assertion(
            !scfg->isRunning(),
            "expected server not to be running" );

    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
        return -1;
    }
}
