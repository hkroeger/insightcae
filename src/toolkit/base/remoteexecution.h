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

    /**
     * @brief RemoteExecutionConfig
     * create a copy
     * @param orec
     */
    RemoteExecutionConfig(const RemoteExecutionConfig& orec);

    /**
     * @brief RemoteExecutionConfig
     * read properties from config file
     * @param location
     * @param localREConfigFile
     */
    RemoteExecutionConfig(const boost::filesystem::path& location,
                          const boost::filesystem::path& localREConfigFile = "");

    /**
     * @brief RemoteExecutionConfig
     * Attach remote location to local work dir. Save remote location to file in local dir.
     * @param location
     * @param rloc
     * @param localREConfigFile
     */
    RemoteExecutionConfig(const boost::filesystem::path& location,
                          const RemoteLocation& rloc,
                          const boost::filesystem::path& localREConfigFile = "");

    /**
     * @brief RemoteExecutionConfig
     * Attach remote location (server / directory) to local work dir.
     * Save remote location to file in local dir.
     * @param rsc
     * @param location
     * @param remotePath
     * @param localREConfigFile
     */
    RemoteExecutionConfig(RemoteServer::ConfigPtr rsc,
                          const boost::filesystem::path& location,
                          const boost::filesystem::path& remotePath = "",
                          const boost::filesystem::path& localREConfigFile = "");


    const boost::filesystem::path& localDir() const;
    const boost::filesystem::path& metaFile() const;

    void cleanup(bool forceRemoval=false) override;

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
    void writeConfig(const boost::filesystem::path& localREConfigFile = "") const;

};




}


#endif // REMOTEEXECUTION_H
