#include "base/remoteexecution.h"
#include "base/sshlinuxserver.h"

using namespace insight;

int main(int /*argc*/, char* /*argv*/[])
{
  try
  {

    RemoteServer::ConfigPtr reccfg = std::make_shared<SSHLinuxServer::Config>(
          "/home/hannes/tmp/analyses_basedir", //boost::filesystem::current_path(),
          "localhost"
          );

    insight::dbg()<<"create"<<std::endl;
    RemoteExecutionConfig rec(reccfg, ".");


    std::cout << "localDir = "<<rec.localDir()<<std::endl;
    auto rd = rec.remoteDir();
    std::cout << "remoteDir = "<<rd<<std::endl;
    std::cout << "remoteDirExists = "<<rec.remoteDirExists()<<std::endl;


    auto process = rec.server()->launchBackgroundProcess(
          "analyze "
          "--workdir=\""+rd.string()+"\" "
          "--server"
        );

    process->kill();

  }
  catch (const std::exception& e)
  {
    std::cerr<<"Error occurred: "<<e.what()<<std::endl;
    return -1;
  }

  return 0;
}
