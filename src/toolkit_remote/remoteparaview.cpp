#include "remoteparaview.h"

#include "base/cppextensions.h"

namespace insight {




RemoteParaview::RemoteParaview(
        const RemoteExecutionConfig &rec,
        const boost::filesystem::path &stateFile,
        const boost::filesystem::path &remoteSubDir )
{
    insight::assertion( rec.isActive(), "remote location is not available" );

    remotePort_ = rec.server()->findFreeRemotePort();
    portMapping_ = rec.server()->makePortsAccessible( { remotePort_ }, {} );

    boost::filesystem::path dir = rec.remoteDir();
    if (!remoteSubDir.empty())
        dir = dir / remoteSubDir;

    std::string caseLabel;
    {
        auto cd = boost::filesystem::canonical(rec.localDir());
        caseLabel = cd.filename().string();
        for (int i=0; i<2; ++i)
        {
            cd=cd.parent_path();
            dbg()<<"cd="<<cd<<endl;
            if (cd.empty()) break;
            caseLabel = cd.filename().string()+"_"+caseLabel;
        }
    }

    if (!stateFile.empty())
    {
        loadScript_=std::make_unique<TemporaryFile>(
                    "load-remote-PV-%%%%.py", rec.localDir() );

        loadScript_->stream() << str(boost::format(
         "import paraview.simple\n"
         "paraview.simple.LoadState('%s',"
               "LoadStateDataFileOptions='Search files under specified directory',"
               "DataDirectory='%s/system' )\n"
         ) % stateFile.generic_path().string() % dir.generic_path().string() )
         ;
        loadScript_->closeStream();
    }


    // launch remote PV server
    remotePVServer_ =
            rec.server()->launchBackgroundProcess(
                str(boost::format(
                    "cd \"%s\";"
                    "pvserver-offscreen --use-offscreen-rendering --server-port=%d"
                ) % dir.generic_path().string()
                  % remotePort_ )
              );


    // launch local client
    std::vector<std::string> pvcArgs = {
        str(boost::format("--server-url=cs://127.0.0.1:%d")
                % portMapping_->localListenerPort(remotePort_) )
    };

#ifndef WIN32
    pvcArgs.insert( pvcArgs.begin(), { "--title", caseLabel } );
#endif

    if (stateFile.empty())
    {
        pvcArgs.push_back(
                    str(boost::format("--data=%s/system/controlDict")
                        % dir.generic_path().string() )
                    );
    }
    else
    {
        pvcArgs.push_back(
                    str(boost::format("--state=%s")
                        % loadScript_->path().string() )
                    );
    }

    {
        auto& os=dbg();
        os<<"launching "<<boost::process::search_path("paraview");
        for (const auto& a: pvcArgs)
            os<<" \""<<a<<"\"";
        os<<std::endl;
    }

    localPVClient_ =
            std::make_unique<boost::process::child>(
                boost::process::search_path("paraview"),
                pvcArgs
          );
    if (!localPVClient_->running())
      throw insight::Exception("Failed to launch paraview!");

}




void RemoteParaview::wait()
{
    localPVClient_->wait();
    localPVClient_.reset();
}




RemoteParaview::~RemoteParaview()
{
    if (localPVClient_ && localPVClient_->running())
    {
        localPVClient_->terminate();
        wait();
    }

    try
    {
        remotePVServer_->kill();
    }
    catch (const std::exception& ex)
    {
        insight::Warning(
                    std::string("Could not kill process on remote server! Reason: ")
                    + ex.what() );
    }
}




} // namespace insight
