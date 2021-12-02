#ifndef INSIGHT_REMOTEPARAVIEW_H
#define INSIGHT_REMOTEPARAVIEW_H


#include "base/boost_include.h"
#include "base/remoteexecution.h"
#include "base/tools.h"
#include "base/externalprocess.h"
#include "openfoam/paraview.h"

namespace insight {

boost::filesystem::path remotePath(
        const RemoteExecutionConfig &rec,
        const boost::filesystem::path& remoteSubDir = ""
        );

class RemotePVServer
{
    int remotePort_;
    RemoteServer::PortMappingPtr portMapping_;
    RemoteServer::BackgroundJobPtr remotePVServer_;

public:
    RemotePVServer(
            const RemoteExecutionConfig &rec,
            const boost::filesystem::path& remoteSubDir = "" );
    ~RemotePVServer();

    int localListenerPort() const;
};



class RemoteParaview
        : public RemotePVServer,
          public Paraview
{

public:
    RemoteParaview(
            const insight::RemoteExecutionConfig& rec,
            const boost::filesystem::path& stateFile = "",
            const boost::filesystem::path& remoteSubDir = ""
            );
};

} // namespace insight

#endif // INSIGHT_REMOTEPARAVIEW_H
