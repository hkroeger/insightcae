#ifndef INSIGHT_REMOTELOCATION_H
#define INSIGHT_REMOTELOCATION_H

#include "base/boost_include.h"
#include "base/taskspoolerinterface.h"
#include "boost/process.hpp"

#include "remoteserverlist.h"




namespace insight
{




/**
 * @brief The RemoteLocation class
 * points to a directory on a remote host
 * can be inactive (i.e. remote directory does not yet exist; remote host is not running)
 * or active (directory exists, remote host is up)
 */
class RemoteLocation
{
protected:
  RemoteServer::ConfigPtr serverConfig_;
  RemoteServerPtr serverInstance_;

  /**
   * @brief remoteDir_
   * basolute path on remote host
   */
  boost::filesystem::path remoteDir_;
  bool emptyRemotePathSupplied_, autoCreateRemoteDir_;


//    std::vector<boost::process::child> tunnelProcesses_;

//    void runRsync
//    (
//        const std::vector<std::string>& args,
//        std::function<void(int progress,const std::string& status_text)> progress_callback =
//            std::function<void(int,const std::string&)>()
//    );

//    virtual void launchHost();
    virtual void removeRemoteDir();
//    virtual void disposeHost();

    void initialize();

    bool isActive_;
    void validate();
    void assertActive() const;

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
     * @param rsc
     * remote server configuration
     * @param remotePath
     * directory (absolute on remote machine). If empty, it will be auto-created and its name can be queried using remoteDir().
     */
    RemoteLocation(RemoteServer::ConfigPtr rsc, const boost::filesystem::path& remotePath = "", bool autoCreateRemoteDir = true);

    virtual ~RemoteLocation();

    /**
     * @brief createTunnels
     * create tunnels to remote location
     * @param remoteListenPorts
     * Will be translated to a subset of SSH's -R option:
     * Specifies that connections to the given TCPsecond port or Unix socket on the remote (server) host are to be
     * forwarded to the local side.
     * This works by allocating a socket to listen to either a TCP port or to a Unix socket on the remote side.
     * -R localport:host:hostport
     *
     * @param localListenPorts
     * Will be translated to a subset of SSH's -L option:
     * Specifies that connections to the given TCP port or Unix socket on the local (client) host are to be
     * forwarded to the given host and port, or Unix socket, on the remote side.
     * -L localport:host:hostport
     *
     */

    // get from server()
//    void createTunnels(
//        std::vector<boost::tuple<int,std::string,int> > remoteListenPorts = {},
//        std::vector<boost::tuple<int,std::string,int> > localListenPorts = {}
//        );

//    void stopTunnels();

    // ====================================================================================
    // ======== query functions
    RemoteServerPtr server() const;
    RemoteServer::ConfigPtr serverConfig() const;
    const boost::filesystem::path& remoteDir() const;


    // ====================================================================================
    // ======== init /deinit

    virtual void cleanup();

    // ==================================openfoam==================================================
    // ======== query remote location

//    bool serverIsUp() const;
    /**
     * @brief remoteDirExists
     * checks, if remote dir is existing
     * @return
     */
    bool remoteDirExists() const;

    std::vector<bfs_path> remoteLS() const;
    std::vector<bfs_path> remoteSubdirs() const;


    std::string remoteSourceOFEnvStatement() const;


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
        const boost::filesystem::path& remoteDir
        );

    bool isActive() const;
};




} // namespace insight

#endif // INSIGHT_REMOTELOCATION_H
