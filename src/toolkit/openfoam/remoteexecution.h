#ifndef REMOTEEXECUTION_H
#define REMOTEEXECUTION_H

#include "base/boost_include.h"
#include "boost/process.hpp"
#include "openfoam/taskspoolerinterface.h"
#include "openfoam/remoteserverlist.h"

namespace insight
{



class RemoteExecutionConfig
{
protected:
    bfs_path meta_file_;
    std::string server_;
    boost::filesystem::path localDir_, remoteDir_;

    boost::filesystem::path socket() const;

    void execRemoteCmd(const std::string& cmd);
    void runRsync
    (
        const std::vector<std::string>& args,
        std::function<void(int progress,const std::string& status_text)> progress_callback = std::function<void(int,const std::string&)>()
    );

public:
    RemoteExecutionConfig(const boost::filesystem::path& location, bool needConfig=true, const bfs_path& meta_file="");

    const std::string& server() const;
    const boost::filesystem::path& localDir() const;
    const boost::filesystem::path& remoteDir() const;

    const boost::filesystem::path& metaFile() const;

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
    void putFile
    (
        const boost::filesystem::path& localFile,
        const boost::filesystem::path& remoteFileName,
        std::function<void(int progress,const std::string& status_text)> progress_callback = std::function<void(int,const std::string&)>()
    );

    void syncToRemote
    (
        const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
        std::function<void(int progress,const std::string& status_text)> progress_callback = std::function<void(int,const std::string&)>()
    );
    void syncToLocal
    (
        bool skipTimeSteps=false,
        const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
        std::function<void(int progress,const std::string& status_text)> progress_callback = std::function<void(int,const std::string&)>()
    );

    void queueRemoteCommand(const std::string& command, bool waitForPreviousFinished=true);
    void waitRemoteQueueFinished();
    void waitLastCommandFinished();

    void cancelRemoteCommands();
    void removeRemoteDir();

    /**
     * @brief isValid
     * checks, if configuration data is valid (not if remote dir really exists)
     * @return
     */
    bool isValid() const;

    /**
     * @brief remoteDirExists
     * checks, if remote dir is existing
     * @return
     */
    bool remoteDirExists() const;
};

}

#endif // REMOTEEXECUTION_H
