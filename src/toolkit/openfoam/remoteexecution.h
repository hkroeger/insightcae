#ifndef REMOTEEXECUTION_H
#define REMOTEEXECUTION_H

#include "base/boost_include.h"

namespace insight
{

class RemoteExecutionConfig
{
    std::string server_;
    boost::filesystem::path localDir_, remoteDir_;

public:
    RemoteExecutionConfig(const boost::filesystem::path& location);

    const std::string& server() const;
    const boost::filesystem::path& localDir() const;
    const boost::filesystem::path& remoteDir() const;

    void syncToRemote();
    void syncToLocal();

    void executeRemote(const std::string& command);
};

}

#endif // REMOTEEXECUTION_H
