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

        // allocate single server
        auto srvcfg = remoteServers.findServer(
            "docker_test_pool/dcksrv1"
            //"localhost"
            );

        //expect no instance to be there yet
        insight::assertion(
            !bool(srvcfg->getInstanceIfRunning()),
            "expected no instance to be present" );

        {
            // launch
            auto srv=srvcfg->instance();

            insight::assertion(
                srvcfg->isRunning(),
                "machine was expected to be running but is not!" );

            srv->executeCommand("ls -l; pwd", true);

            int np = srv->config().occupiedProcessors();
            std::cout<<"np occupied = "<<np<<std::endl;

            int npu = srv->config().unoccupiedProcessors();
            std::cout<<"np unoccupied = "<<npu<<std::endl;

            // deallocate server
        }

        insight::assertion(
            !srvcfg->isRunning(),
            "machine was expected to be stopped but is not!" );

    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
        return -1;
    }
}
