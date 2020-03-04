#ifndef REMOTEEXECUTION_H
#define REMOTEEXECUTION_H

#include "base/boost_include.h"
#include "boost/process.hpp"
#include "openfoam/taskspoolerinterface.h"
#include "openfoam/remoteserverlist.h"

namespace insight
{




bool hostAvailable(const std::string& host);




class RemoteLocation
{
protected:
    boost::filesystem::path launchScript_;
    std::string server_;
    boost::filesystem::path remoteDir_;
    bool autoCreateRemoteDirRequired_;

    bool isValid_;

    void runRsync
    (
        const std::vector<std::string>& args,
        std::function<void(int progress,const std::string& status_text)> progress_callback =
            std::function<void(int,const std::string&)>()
    );

    virtual void launchHost();
    virtual void validate();
    virtual void removeRemoteDir();
    virtual void disposeHost();

    void initialize();

    void assertValid() const;

public:
    /**
     * @brief RemoteLocation
     * copies configuration from another remote location container
     * @param orec
     */
    RemoteLocation(const RemoteLocation& orec);

    /**
     * @brief RemoteLocation
     * read settings of existing remote location from configuration file
     * @param mf
     */
    RemoteLocation(const boost::filesystem::path& mf);

    /**
     * @brief RemoteLocation
     * Construct from remote server info
     * @param rsi
     * remote server
     * @param remoteRelPath
     * directory (absolute on remote machine). If empty, it will be auto-created and its name can be queried using remoteDir().
     */
    RemoteLocation(const RemoteServerInfo& rsi, const boost::filesystem::path& remotePath);



    // ====================================================================================
    // ======== query functions
    const std::string& server() const;
    const boost::filesystem::path& remoteDir() const;


    // ====================================================================================
    // ======== init /deinit

    void cleanup();

    // ====================================================================================
    // ======== query remote location

    bool serverIsUp() const;
    /**
     * @brief remoteDirExists
     * checks, if remote dir is existing
     * @return
     */
    bool remoteDirExists() const;

    std::vector<bfs_path> remoteLS() const;
    std::vector<bfs_path> remoteSubdirs() const;




    // ====================================================================================
    // ======== action functions

    virtual int execRemoteCmd(const std::string& cmd, bool throwOnFail=true);

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
        std::function<void(int progress,const std::string& status_text)> progress_callback =
                            std::function<void(int,const std::string&)>()
    );

    virtual void syncToRemote
    (
        const boost::filesystem::path& localDir,
        const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
        std::function<void(int progress,const std::string& status_text)> progress_callback =
                            std::function<void(int,const std::string&)>()
    );
    virtual void syncToLocal
    (
        const boost::filesystem::path& localDir,
        bool skipTimeSteps=false,
        const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
        std::function<void(int progress,const std::string& status_text)> progress_callback =
                            std::function<void(int,const std::string&)>()
    );


    // ====================================================================================
    // ======== queue commands

    boost::filesystem::path socket() const;

    void queueRemoteCommand(const std::string& command, bool waitForPreviousFinished=true);
    void waitRemoteQueueFinished();
    void waitLastCommandFinished();

    void cancelRemoteCommands();


    static void writeConfigFile(
        const boost::filesystem::path& cfgf,
        const std::string& server,
        const boost::filesystem::path& remoteDir,
        const boost::filesystem::path& launchScript = ""
        );

    bool isValid() const;
};




class RemoteExecutionConfig
    : public RemoteLocation
{
protected:
    boost::filesystem::path localREConfigFile_;
    boost::filesystem::path localDir_;


public:
    RemoteExecutionConfig(const RemoteExecutionConfig& orec);

    RemoteExecutionConfig(const boost::filesystem::path& location,
                          const boost::filesystem::path& localREConfigFile = "");

    RemoteExecutionConfig(const RemoteServerInfo& rsi,
                          const boost::filesystem::path& location,
                          const boost::filesystem::path& remoteRelPath = "",
                          const boost::filesystem::path& localREConfigFile = "");

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
