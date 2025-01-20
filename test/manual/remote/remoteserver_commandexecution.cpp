#include "base/boost_include.h"

#include "base/exception.h"
#include "base/remoteserver.h"
#include "base/remotelocation.h"
#include "base/sshlinuxserver.h"
#include "base/remoteserverlist.h"

#include "dummyanalysis.h"


using namespace std;
using namespace insight;
using namespace boost;

#define TEST_SERVER_RUNNING

int main()
{
    try
    {

        // allocate single server
        auto srvcfg = remoteServers.findServer(
            "docker_test_pool/dcksrv1"
            //"localhost"
            );

#if (!defined(TEST_SERVER_RUNNING))
        //expect no instance to be there yet
        insight::assertion(
            !bool(srvcfg->getInstanceIfRunning()),
            "expected no instance to be present" );
#endif

        {
            // launch
            auto rc=RemoteLocation(srvcfg);
            rc.initialize();
            auto &srv=*rc.server();


            auto params = DummyAnalysis::Parameters::makeDefault();
            params->pack();

            std::cout<<"LOCAL\n"<<std::endl;
            params->saveToStream(std::cout, ".");

            auto remfp=rc.remoteDir()/"input.ist";
            {
                auto remf=srv.remoteOFStream(remfp, 1);
                params->saveToStream(remf->stream(), ".");
            }

            std::cout<<"REMOTE\n"<<std::endl;
            insight::assertion(
                srv.executeCommand("cat "+remfp.string(), true)==0,
                "cat command failed" );


            auto proc = srv.launchBackgroundProcess(
                "analyze "
                " --workdir=\""+insight::toUnixPath(rc.remoteDir())+"\""
                " --server"
                +str(
                    format(" --port %d") % 12345
                )+
                +" "+remfp.string()+
                " >\""+insight::toUnixPath(rc.remoteDir()/"analyze.log")+"\" 2>&1 </dev/null"
                );


            // rc.server()->detach();
            // std::vector<insight::RemoteServer::ExpectedOutput> eobd;

            // boost::process::ipstream is;

            // auto process = srv->launchCommand(
            //     "cat - & echo PID===$!===PID",
            //     //#ifdef WIN32
            //     boost::process::std_out > is
            //     //#else
            //     //        boost::process::std_err > is
            //     //#endif
            //     , boost::process::std_in < boost::process::null
            //     );

            // insight::assertion(
            //     process->running(),
            //     "could not start background process");

            // std::vector<std::string> pidMatch;

            // std::vector<insight::RemoteServer::ExpectedOutput> pats(eobd.begin(), eobd.end());
            // pats.push_back( { boost::regex("PID===([0-9]+)===PID"), &pidMatch } );
            // srv->lookForPattern(is, pats);

            // std::cout<<pidMatch[1]<<std::endl;
            // int remotePid=boost::lexical_cast<int>(pidMatch[1]);

            // insight::dbg()<<"remote process PID = "<<remotePid<<std::endl;


            // auto p= std::make_shared<insight::SSHLinuxServer::BackgroundJob>(*srv, remotePid );

            // deallocate server
#if (!defined(TEST_SERVER_RUNNING))
            srv.destroyIfPossible();
#endif

        }

#if (!defined(TEST_SERVER_RUNNING))
        insight::assertion(
            !srvcfg->isRunning(),
            "machine was expected to be stopped but is not!" );
#endif
    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
        return -1;
    }
}
