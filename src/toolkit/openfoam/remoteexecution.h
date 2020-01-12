#ifndef REMOTEEXECUTION_H
#define REMOTEEXECUTION_H

#include "base/boost_include.h"
#include "boost/process.hpp"
#include "openfoam/taskspoolerinterface.h"
#include "openfoam/remoteserverlist.h"

namespace insight
{


class RemoteLocation
{
protected:
    std::string server_;
    boost::filesystem::path remoteDir_;

    void runRsync
    (
        const std::vector<std::string>& args,
        std::function<void(int progress,const std::string& status_text)> progress_callback = std::function<void(int,const std::string&)>()
    );

public:
    RemoteLocation(const RemoteLocation& orec);
    RemoteLocation(const boost::filesystem::path& mf);
    RemoteLocation(const std::string& hostName, const boost::filesystem::path& remoteDir);

    const std::string& server() const;
    const boost::filesystem::path& remoteDir() const;

    std::vector<bfs_path> remoteLS() const;
    std::vector<bfs_path> remoteSubdirs() const;

    /**
     * @brief putFile
     * copy single file to remote
     * @param localFile
     * path to local file
     * @param remmoteFileName
     * path to remote file, relative to remote dir
     */
    virtual void putFile
    (
        const boost::filesystem::path& localFile,
        const boost::filesystem::path& remoteFileName,
        std::function<void(int progress,const std::string& status_text)> progress_callback = std::function<void(int,const std::string&)>()
    );

    virtual void syncToRemote
    (
        const boost::filesystem::path& localDir,
        const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
        std::function<void(int progress,const std::string& status_text)> progress_callback = std::function<void(int,const std::string&)>()
    );
    virtual void syncToLocal
    (
        const boost::filesystem::path& localDir,
        bool skipTimeSteps=false,
        const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
        std::function<void(int progress,const std::string& status_text)> progress_callback = std::function<void(int,const std::string&)>()
    );

    virtual void removeRemoteDir();

    /**
     * @brief remoteDirExists
     * checks, if remote dir is existing
     * @return
     */
    bool remoteDirExists() const;
};




class RemoteExecutionConfig
    : public RemoteLocation
{
protected:
    bfs_path meta_file_;
    boost::filesystem::path localDir_;

    boost::filesystem::path socket() const;

    void execRemoteCmd(const std::string& cmd);

public:
    RemoteExecutionConfig(const RemoteExecutionConfig& orec);
    RemoteExecutionConfig(const boost::filesystem::path& location, bool needConfig=true, const bfs_path& meta_file="");

    const boost::filesystem::path& localDir() const;
    const boost::filesystem::path& metaFile() const;

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
        std::function<void(int progress,const std::string& status_text)> progress_callback = std::function<void(int,const std::string&)>()
    ) override;

    virtual void syncToRemote
    (
        const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
        std::function<void(int progress,const std::string& status_text)> progress_callback = std::function<void(int,const std::string&)>()
    );

    virtual void syncToLocal
    (
        bool skipTimeSteps=false,
        const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
        std::function<void(int progress,const std::string& status_text)> progress_callback = std::function<void(int,const std::string&)>()
    );

    void queueRemoteCommand(const std::string& command, bool waitForPreviousFinished=true);
    void waitRemoteQueueFinished();
    void waitLastCommandFinished();

    void cancelRemoteCommands();
    void removeRemoteDir() override;

    /**
     * @brief isValid
     * checks, if configuration data is valid (not if remote dir really exists)
     * @return
     */
    bool isValid() const;

};

}

#endif // REMOTEEXECUTION_H
