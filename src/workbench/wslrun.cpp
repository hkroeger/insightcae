#include "base/tools.h"
#include "base/remoteexecution.h"

#include "wslrun.h"
#include "analysisform.h"
#include "ui_analysisform.h"


#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include "boost/process.hpp"

using namespace std;
using namespace boost;
namespace bf=boost::filesystem;
namespace bp=boost::process;



void WSLRun::launchRemoteAnalysisServer()
{
//  auto rd = remote_.remoteDir();

//  int ret = remote_.execRemoteCmd(
//        "analyze "
//        "--workdir=\""+rd.string()+"\" "
//        "--server >\""+rd.string()+"/server.log\" 2>&1 </dev/null &"
//      );

//  if (ret!=0)
    throw insight::Exception("Failed to launch remote analysis server executable!");
}






WSLRun::WSLRun(AnalysisForm *af, const boost::filesystem::path &WSLExecutable, bool resume)
  : RemoteRun(
      af,
      insight::RemoteExecutionConfig(
        insight::remoteServers.findServer("localhost").second,
        af->localCaseDirectory() ),
      resume ),
    WSLExecutable_( WSLExecutable )
{}

std::unique_ptr<WSLRun> WSLRun::create(AnalysisForm *af, const boost::filesystem::path &WSLExecutable, bool resume)
{
  std::unique_ptr<WSLRun> a(new WSLRun(af, WSLExecutable, resume));
  a->launch();
  return a;
}



