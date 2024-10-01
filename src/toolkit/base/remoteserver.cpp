#include "remoteserver.h"

#include <cstdlib>
#include <regex>

#include "base/exception.h"
#include "base/tools.h"
#include "base/sshlinuxserver.h"
#include "base/wsllinuxserver.h"
#include "base/rapidxml.h"
#include "openfoam/openfoamcase.h"

#include "rapidxml/rapidxml_print.hpp"

using namespace std;
using namespace boost;

namespace insight {


RemoteServer::Config::Config(const boost::filesystem::path& bp, int np)
    : defaultDirectory_(bp), np_(np), originatedFromExpansion_(false)
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

std::shared_ptr<RemoteServer> RemoteServer::Config::getInstanceIfRunning() const
{
    if (isRunning())
    {
        return instance();
    }
    else
        return nullptr;
}

int RemoteServer::Config::unoccupiedProcessors() const
{
    if (isRunning())
    {
        int npTotal;
        int nu=occupiedProcessors(&npTotal);
        return npTotal-nu;
    }
    else
        return np_;
}


bool RemoteServer::Config::isUnoccupied() const
{
    if (isRunning())
    {
        if (unoccupiedProcessors()>=np_)
            return true;
        else
            return false;
    }
    else
        return true;
}

bool RemoteServer::Config::isExpandable() const
{
    return false;
}

RemoteServer::ConfigPtr RemoteServer::Config::expanded(int id) const
{
    return nullptr;
}



RemoteServer::RemoteServer()
{}


RemoteServer::~RemoteServer()
{}

void RemoteServer::destroyIfPossible()
{}


string RemoteServer::serverLabel() const
{
    return config();
}


void RemoteServer::lookForPattern(
        std::istream &is,
        const std::vector<ExpectedOutput> &pattern )
{
    std::vector<bool> found(pattern.size(), false);
    auto allFound = [&] () -> bool
    {
        bool all=true;
        for (const auto& f: found)
        {
            all = all && f;
        }
        return all;
    };

    int linesRead = 0;
    while ( !allFound() && (linesRead<100*(1+pattern.size())) )
    {
      std::string line;
      if (getline(is, line))
      {
        linesRead++;

        for (int i=0; i<pattern.size(); ++i)
        {
            if (!found[i])
            {
                auto pat = pattern[i];
                boost::smatch matches;
                if (boost::regex_match(line, matches, pat.first))
                {
                    if (pat.second)
                    {
                        pat.second->clear();
                        for (std::string match : matches)
                        {
                            pat.second->push_back(match);
                        }
                    }
                    found[i]=true;
                }
            }
        }
      }
    }
}


RemoteServer::RemoteStream::~RemoteStream()
{}



void RemoteServer::setTransferBandWidthLimit(int kBPerSecond)
{}

int RemoteServer::transferBandWidthLimit() const
{
    return -1;
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


RemoteServerPoolConfig::RemoteServerPoolConfig(rapidxml::xml_node<> *e)
{
    if (auto rst = configTemplate_=RemoteServer::Config::create(e))
    {
        if (auto *a = e->first_attribute("maxSize"))
            maxSize_=insight::toNumber<int>(a->value());
        else
            throw insight::Exception(
                "no maximum server pool size specified for pool %s",
                rst->c_str() );

        if (auto *a = e->first_attribute("np"))
            np_=insight::toNumber<int>(a->value());
        else
            throw insight::Exception(
                "no processor count specified for pool %s",
                rst->c_str() );
    }
    else
    {
        std::string label("(unlabelled)");
        if (auto *le=e->first_attribute("label"))
            label=std::string(le->value());
        throw insight::Exception(
            "ignored invalid remote machine pool configuration: %s",
            label.c_str());
    }
}

void RemoteServerPoolConfig::save(
    rapidxml::xml_node<> *e,
    rapidxml::xml_document<>& doc ) const
{
    configTemplate_->save(e, doc);
    appendAttribute(doc, *e, "maxSize", maxSize_);
    appendAttribute(doc, *e, "np", np_);
}

bool operator<(const RemoteServerPoolConfig& p1, const RemoteServerPoolConfig& p2)
{
    return p1.configTemplate_<p2.configTemplate_;
}


} // namespace insight
