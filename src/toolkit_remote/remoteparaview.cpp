#include "remoteparaview.h"

#include "base/cppextensions.h"

namespace insight {


boost::filesystem::path remotePath(
        const RemoteExecutionConfig &rec,
        const boost::filesystem::path& remoteSubDir
        )
{
    boost::filesystem::path dir = rec.remoteDir();
    if (!remoteSubDir.empty())
        dir = dir / remoteSubDir;
    return dir;
}



RemotePVServer::RemotePVServer(
        const RemoteExecutionConfig &rec,
        const boost::filesystem::path &remoteSubDir)
{
    insight::assertion( rec.isActive(), "remote location is not available" );

    remotePort_ = rec.server()->findFreeRemotePort();
    portMapping_ = rec.server()->makePortsAccessible( { remotePort_ }, {} );

    // launch remote PV server
    remotePVServer_ =
            rec.server()->launchBackgroundProcess(
                str(boost::format(
                    "cd \"%s\";"
                    "pvserver-offscreen --use-offscreen-rendering --server-port=%d"
                ) % remotePath(rec, remoteSubDir).generic_path().string()
                  % remotePort_ )
              );
}


RemotePVServer::~RemotePVServer()
{
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



int RemotePVServer::localListenerPort() const
{
    return portMapping_->localListenerPort(remotePort_);
}




RemoteParaview::RemoteParaview(
        const RemoteExecutionConfig &rec,
        const boost::filesystem::path &stateFile,
        const boost::filesystem::path &remoteSubDir )

    : RemotePVServer(rec, remoteSubDir),
      Paraview(rec.localDir(), stateFile,
               false, false, false, true, 0, 1e10,
               {
                str(boost::format("--server-url=cs://127.0.0.1:%d")
                % localListenerPort() )
               },
               remotePath(rec, remoteSubDir)
               )
{}





} // namespace insight
