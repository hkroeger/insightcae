#ifndef INSIGHT_REMOTESERVER_H
#define INSIGHT_REMOTESERVER_H


#include "base/boost_include.h"
#include "base/exception.h"

#include "boost/process.hpp"
#include "boost/process/child.hpp"
#include "boost/process/async.hpp"

#include "rapidxml/rapidxml.hpp"
namespace insight {





/**
 * @brief The RemoteServer class
 * represents a running server.
 * Server is brought up, if needed, when an object of this class is created.
 * If destructable (e.g. containers), this is not done upon
 * object destruction but has to be executed explicitly.
 */
class RemoteServer
{
public:

  /**
   * @brief The Config class
   * stores server info, the base class stores the label
   * Represents all the information that can be provided without the server actually running
   */

  struct Config;
  typedef std::shared_ptr<Config> ConfigPtr;

  struct Config : public std::string
  {
    boost::filesystem::path defaultDirectory_;
    int np_;
    bool originatedFromExpansion_;

    Config(const boost::filesystem::path& bp, int np);

    virtual bool isDynamicallyAllocated() const =0;

    /**
     * @brief create
     * read configuration
     * @param e
     * @return
     */
    static std::shared_ptr<Config> create(rapidxml::xml_node<> *e);

    /**
     * @brief getInstanceIfRunning
     * attempt to connect
     * @return
     * nullptr, if not running
     */
    virtual std::shared_ptr<RemoteServer> getInstanceIfRunning() const;

    // some query functions

    /**
     * @brief checkIfRunning
     * check, if the server is actually running
     * @return
     */
    virtual bool isRunning() const =0;

    /**
     * @brief occupiedProcessors
     * check, how many processors on the machine are occupied by processes
     * @return
     */
    virtual int occupiedProcessors(int* nProcAvail=nullptr) const =0;

    /**
     * @brief unoccupiedProcessors
     * return, how many processors are free on the machine
     * @return
     */
    int unoccupiedProcessors() const;

    bool isUnoccupied() const;

    /**
     * @brief instance
     * attempts to connect and launches a new instance, if necessary
     * @return
     * instance or exception, if instance could not be launched
     */
    virtual std::shared_ptr<RemoteServer> instance() const =0;


    virtual std::pair<boost::filesystem::path,std::vector<std::string> >
    commandAndArgs(const std::string& command) const =0;

    template<typename ...Args>
    int executeCommand(const std::string& command, Args&&... addArgs) const
    {
        insight::CurrentExceptionContext ex(
            "executing command \"%s\" on remote server %s",
            command.c_str(), c_str() );

        auto c_and_a = commandAndArgs(command);
        int ret = boost::process::system(
            c_and_a.first, boost::process::args(c_and_a.second),
            std::forward<Args>(addArgs)...
            );
        return ret;
    }

    virtual void save(rapidxml::xml_node<> *e, rapidxml::xml_document<>& doc) const =0;

    virtual ConfigPtr clone() const =0;

    virtual bool isExpandable() const;
    virtual ConfigPtr expanded(int id) const;
    inline bool wasExpanded() const { return originatedFromExpansion_; }

    virtual bool isDynamicallyCreatable() const =0;
    virtual bool isDynamicallyDestructable() const =0;
  };


public:

  RemoteServer();
  virtual ~RemoteServer();

  virtual void destroyIfPossible();

  virtual const Config& config() const =0;
  std::string serverLabel() const;

  /**
   * @brief assertRunning
   * check if the server is actually really running.
   * Throw an exception, if not.
   */
  void assertRunning() const
  {
      insight::assertion(
          config().isRunning(),
          "the remote machine %s was expected to be running but is not",
          serverLabel().c_str()
          );
  }

  template<typename ...Args>
  int executeCommand(const std::string& command, bool throwOnFail, Args&&... addArgs) const
  {
      if (throwOnFail) assertRunning();

      auto ret = config().executeCommand(
          command, std::forward<Args>(addArgs)...);

      if ( throwOnFail && (ret != 0) )
      {
          throw insight::Exception(
              "Could not execute command on server %s: \"%s\"",
              serverLabel().c_str(), command.c_str() );
      }

      return ret;
  }

  template<class ...Args>
  std::unique_ptr<boost::process::child>
  launchCommand(const std::string& command, Args&&... addArgs)
  {
    insight::CurrentExceptionContext ex("executing command on remote host: "+command);
    assertRunning();

    auto c_and_a = config().commandAndArgs(command);
    return std::unique_ptr<boost::process::child>(new boost::process::child(
                c_and_a.first, boost::process::args(c_and_a.second),
                std::forward<Args>(addArgs)...
          ));
  }




  struct BackgroundJob
  {
  protected:
    RemoteServer& server_;
  public:
    BackgroundJob(RemoteServer& server);
    virtual void kill() =0;
  };
  typedef std::shared_ptr<BackgroundJob> BackgroundJobPtr;

  typedef std::pair<boost::regex, std::vector<std::string>*> ExpectedOutput;

  static void lookForPattern(
          std::istream& is,
          const std::vector<ExpectedOutput>& expectedOutputBeforeDetach
          );

  virtual BackgroundJobPtr launchBackgroundProcess(
          const std::string& cmd,
          const std::vector<ExpectedOutput>& expectedOutputBeforeDetach = {}
          ) =0;




  virtual void putFile
  (
      const boost::filesystem::path& localFilePath,
      const boost::filesystem::path& remoteFilePath,
      std::function<void(int progress,const std::string& status_text)> progress_callback =
                          std::function<void(int,const std::string&)>()
  ) =0;

#ifndef SWIG
  struct RemoteStream {
      virtual ~RemoteStream();
      inline std::ostream& operator()() { return stream(); };
      inline operator std::ostream& () { return stream(); };
      virtual std::ostream& stream() =0;
  };

  virtual std::unique_ptr<RemoteStream> remoteOFStream
    (
        const boost::filesystem::path& remoteFilePath,
        int totalBytes,
        std::function<void(int progress,const std::string& status_text)> progress_callback =
            std::function<void(int,const std::string&)>()
    ) =0;
#endif

  /**
   * @brief setTransferBandWidthLimit
   * set a bandwidth limit for sync transfer
   * @param kBPerSecond
   * maximum bandwidth
   */
  virtual void setTransferBandWidthLimit(int kBPerSecond);

  /**
   * @brief transferBandWidthLimit
   * return the set sync transfer bandwidth limit
   * @return
   */
  virtual int transferBandWidthLimit() const;

  virtual void syncToRemote
  (
      const boost::filesystem::path& localDir,
      const boost::filesystem::path& remoteDir,
      bool includeProcessorDirectories,
      const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
      std::function<void(int progress,const std::string& status_text)> progress_callback =
                          std::function<void(int,const std::string&)>()
  ) =0;

  virtual void syncToLocal
  (
      const boost::filesystem::path& localDir,
      const boost::filesystem::path& remoteDir,
      bool includeProcessorDirectories,
      const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
      std::function<void(int progress,const std::string& status_text)> progress_callback =
                          std::function<void(int,const std::string&)>()
  ) =0;


  virtual bool checkIfDirectoryExists(const boost::filesystem::path& dir) =0;
  virtual boost::filesystem::path getTemporaryDirectoryName(const boost::filesystem::path& templatePath) =0;
  virtual void createDirectory(const boost::filesystem::path& remoteDirectory) =0;
  virtual void removeDirectory(const boost::filesystem::path& remoteDirectory) =0;
  virtual std::vector<boost::filesystem::path> listRemoteDirectory(const boost::filesystem::path& remoteDirectory) =0;
  virtual std::vector<boost::filesystem::path> listRemoteSubdirectories(const boost::filesystem::path& remoteDirectory) =0;

  class PortMapping
  {
  public:
    virtual ~PortMapping();

    virtual int localListenerPort(int remoteListenerPort) const;
    virtual int remoteListenerPort(int localListenerPort) const;
  };

  virtual int findFreeRemotePort() const =0;

  typedef std::shared_ptr<PortMapping> PortMappingPtr;
  virtual PortMappingPtr makePortsAccessible(
      const std::set<int>& remoteListenerPorts,
      const std::set<int>& localListenerPorts
      );

};


typedef std::shared_ptr<RemoteServer> RemoteServerPtr;


class RemoteServerPoolConfig
{
public:
    RemoteServerPoolConfig(rapidxml::xml_node<> *e);

    RemoteServer::ConfigPtr configTemplate_;
    int maxSize_;
    int np_;

    void save(rapidxml::xml_node<> *e, rapidxml::xml_document<>& doc) const;
};

bool operator<(
    const RemoteServerPoolConfig& p1,
    const RemoteServerPoolConfig& p2 );

} // namespace insight

#endif // INSIGHT_REMOTESERVER_H
