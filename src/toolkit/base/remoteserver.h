#ifndef INSIGHT_REMOTESERVER_H
#define INSIGHT_REMOTESERVER_H


#include "base/boost_include.h"
#include "base/exception.h"

#include "boost/process.hpp"
#include "boost/process/child.hpp"
#include "boost/process/async.hpp"

#include "rapidxml/rapidxml.hpp"
namespace insight {


class RemoteServer
{
  bool isRunning_;

protected:
  void assertRunning();

public:

  /**
   * @brief The Config class
   * stores server info, the base class stores the label
   */
  struct Config : public std::string
  {
    boost::filesystem::path defaultDirectory_;

    Config(const boost::filesystem::path& bp);

    virtual bool isDynamicallyAllocated() const =0;

    static std::shared_ptr<Config> create(rapidxml::xml_node<> *e);
    /**
     * @brief getInstanceIfRunning
     * attempt to connect
     * @return
     * nullptr, if not running
     */
    virtual std::shared_ptr<RemoteServer> getInstanceIfRunning() =0;

    /**
     * @brief instance
     * attempts to connect and launches a new instance, if necessary
     * @return
     * instance or exception, if instance could not be launched
     */
    virtual std::shared_ptr<RemoteServer> instance() =0;

    virtual void save(rapidxml::xml_node<> *e, rapidxml::xml_document<>& doc) const =0;
  };

  typedef std::shared_ptr<Config> ConfigPtr;

protected:
  ConfigPtr serverConfig_;

public:
  std::string serverLabel() const;

  virtual bool hostIsAvailable() =0;

  virtual std::pair<boost::filesystem::path,std::vector<std::string> > commandAndArgs(const std::string& command) =0;

  template<typename ...Args>
  int executeCommand(const std::string& command, bool throwOnFail, Args&&... addArgs)
  {
    insight::CurrentExceptionContext ex("executing command on remote host: "+command);
    assertRunning();

    auto c_and_a = commandAndArgs(command);
    int ret = boost::process::system(
                c_and_a.first, boost::process::args(c_and_a.second),
                std::forward<Args>(addArgs)...
          );

    if ( throwOnFail && (ret != 0) )
    {
        throw insight::Exception("Could not execute command on server "+(*serverConfig_)+": \""+command+"\"");
    }

    return ret;
  }

  template<class ...Args>
  std::unique_ptr<boost::process::child>
  launchCommand(const std::string& command, Args&&... addArgs)
  {
    insight::CurrentExceptionContext ex("executing command on remote host: "+command);
    assertRunning();

    auto c_and_a = commandAndArgs(command);
    return std::unique_ptr<boost::process::child>(new boost::process::child(
                c_and_a.first, boost::process::args(c_and_a.second),
                std::forward<Args>(addArgs)...
          ));
  }

  virtual void putFile
  (
      const boost::filesystem::path& localFilePath,
      const boost::filesystem::path& remoteFilePath,
      std::function<void(int progress,const std::string& status_text)> progress_callback =
                          std::function<void(int,const std::string&)>()
  ) =0;

  virtual void syncToRemote
  (
      const boost::filesystem::path& localDir,
      const boost::filesystem::path& remoteDir,
      const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
      std::function<void(int progress,const std::string& status_text)> progress_callback =
                          std::function<void(int,const std::string&)>()
  ) =0;

  virtual void syncToLocal
  (
      const boost::filesystem::path& localDir,
      const boost::filesystem::path& remoteDir,
      const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
      std::function<void(int progress,const std::string& status_text)> progress_callback =
                          std::function<void(int,const std::string&)>()
  ) =0;


  virtual bool checkIfDirectoryExists(const boost::filesystem::path& dir) =0;
  virtual boost::filesystem::path createTemporaryDirectory(const boost::filesystem::path& templatePath) =0;
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

  typedef std::shared_ptr<PortMapping> PortMappingPtr;
  virtual PortMappingPtr makePortsAccessible(
      const std::set<int>& remoteListenerPorts,
      const std::set<int>& localListenerPorts
      );


  virtual void launch();
  virtual bool checkIfRunning() =0;
  virtual void stop();
};


typedef std::shared_ptr<RemoteServer> RemoteServerPtr;


} // namespace insight

#endif // INSIGHT_REMOTESERVER_H
