#ifndef INSIGHT_REMOTEPARAVIEW_H
#define INSIGHT_REMOTEPARAVIEW_H


#include "base/boost_include.h"
#include "base/remoteexecution.h"
#include "base/tools.h"


namespace insight {

class RemoteParaview
{

    int remotePort_;
    RemoteServer::PortMappingPtr portMapping_;
    std::unique_ptr<TemporaryFile> loadScript_;
    RemoteServer::BackgroundJobPtr remotePVServer_;
    std::unique_ptr<boost::process::child> localPVClient_;

public:
    RemoteParaview(
            const insight::RemoteExecutionConfig& rec,
            const boost::filesystem::path& stateFile = "",
            const boost::filesystem::path& remoteSubDir = ""
            );

    void wait();

    ~RemoteParaview();
};

} // namespace insight

#endif // INSIGHT_REMOTEPARAVIEW_H
