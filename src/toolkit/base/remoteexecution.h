#ifndef REMOTEEXECUTION_H
#define REMOTEEXECUTION_H

#include "base/boost_include.h"
#include "boost/process.hpp"
#include "base/remotelocation.h"
#include "base/remoteserverlist.h"




namespace insight
{



/**
 * @brief The RemoteExecutionConfig class
 * stores the connection from a local directory to a remote location,
 * saves the selected connection in a XML file (default meta.foam)
 */
class RemoteExecutionConfig
    : public RemoteLocation
{
protected:
    boost::filesystem::path localREConfigFile_;
    boost::filesystem::path localDir_;


public:
    RemoteExecutionConfig(const RemoteExecutionConfig& orec);

    RemoteExecutionConfig(const boost::filesystem::path& location,
                          const RemoteLocation& rloc,
                          const boost::filesystem::path& localREConfigFile = "");

    RemoteExecutionConfig(const boost::filesystem::path& location,
                          const boost::filesystem::path& localREConfigFile = "");

    RemoteExecutionConfig(RemoteServer::ConfigPtr rsc,
                          const boost::filesystem::path& location,
                          const boost::filesystem::path& remotePath = "",
                          const boost::filesystem::path& localREConfigFile = "");

    ~RemoteExecutionConfig();

    const boost::filesystem::path& localDir() const;
    const boost::filesystem::path& metaFile() const;

    void cleanup() override;

    /**
     * @brief putFile
     * copy single file to remote
     * @param localFile
     * path to local file
     * @param remmoteFileName
     * path to remote file, relative to remote dir
     */
    void putFile
    (
        const boost::filesystem::path& localFile,
        const boost::filesystem::path& remoteFileName,
        std::function<void(int progress,const std::string& status_text)> progress_callback =
                            std::function<void(int,const std::string&)>()
    ) override;

    virtual void syncToRemote
    (
        const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
        std::function<void(int progress,const std::string& status_text)> progress_callback =
                            std::function<void(int,const std::string&)>()
    );

    virtual void syncToLocal
    (
        bool skipTimeSteps=false,
        const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
        std::function<void(int progress,const std::string& status_text)> progress_callback =
                            std::function<void(int,const std::string&)>()
    );


    static boost::filesystem::path defaultConfigFile(const boost::filesystem::path& location);

};




}


#endif // REMOTEEXECUTION_H
