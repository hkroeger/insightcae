#include "remoteserver.h"

#include <cstdlib>
#include <regex>

#include "base/exception.h"
#include "base/tools.h"
#include "base/sshlinuxserver.h"
#include "base/wsllinuxserver.h"
#include "openfoam/openfoamcase.h"

#include "rapidxml/rapidxml_print.hpp"

using namespace std;
using namespace boost;

namespace insight {


RemoteServer::Config::Config(const boost::filesystem::path& bp)
  : defaultDirectory_(bp)
{}

std::shared_ptr<RemoteServer::Config> RemoteServer::Config::create(rapidxml::xml_node<> *e)
{
  std::shared_ptr<RemoteServer::Config> result;
  string label(e->first_attribute("label")->value());

  if (auto *ta = e->first_attribute("type"))
  {
    std::string t(ta->value());
    if (t=="SSHLinux")
    {
      result = std::make_shared<SSHLinuxServer::Config>(e);
    }
    else if (t=="WSLLinux")
    {
      result = std::make_shared<WSLLinuxServer::Config>(e);
    }
  }
  else // type SSH
  {
    result = std::make_shared<SSHLinuxServer::Config>(e);
  }

  if (result)
    static_cast<std::string&>(*result) = label;

  return result;
}


bool RemoteServer::isRunning() const
{
  return isRunning_;
}

void RemoteServer::setRunning(bool isRunning)
{
  isRunning_=isRunning;
}

void RemoteServer::assertRunning() const
{
  insight::assertion(
              isRunning_,
              "the remote machine is not yet started!"
              );
}

RemoteServer::RemoteServer()
  : isRunning_(false)
{}

RemoteServer::Config *RemoteServer::genericServerConfig() const
{
  return serverConfig_.get();
}

std::string RemoteServer::serverLabel() const
{
  return static_cast<std::string>(*serverConfig_);
}





void RemoteServer::launch()
{
  isRunning_=true;
}

void RemoteServer::stop()
{
  isRunning_=false;
}


RemoteServer::PortMapping::~PortMapping()
{}

int RemoteServer::PortMapping::localListenerPort(int remoteListenerPort) const
{
  return remoteListenerPort;
}

int RemoteServer::PortMapping::remoteListenerPort(int localListenerPort) const
{
  return localListenerPort;
}


RemoteServer::PortMappingPtr RemoteServer::makePortsAccessible(
    const std::set<int> &,
    const std::set<int> & )
{
  return std::make_shared<PortMapping>();
}

RemoteServer::BackgroundJob::BackgroundJob(RemoteServer &server)
  : server_(server)
{}



} // namespace insight
