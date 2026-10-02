#include "remoteserver.h"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <thread>
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


std::string shellQuote(const std::string& s)
{
    std::string r="'";
    for (char c: s)
    {
        if (c=='\'')
            r+="'\\''";
        else
            r+=c;
    }
    r+="'";
    return r;
}




RemoteServer::Config::CommandResult
RemoteServer::Config::runCommand(const std::string& command) const
{
    insight::CurrentExceptionContext ex(
        "executing command \"%s\" on remote server %s",
        command.c_str(), c_str() );

    auto c_and_a = commandAndArgs(command);

    // synchronous pipes, stderr is read by a separate thread:
    // (the asynchronous pipes of boost.process 1.65 are not reliable,
    // when several processes are run concurrently)
    boost::process::ipstream out, err;

    boost::process::child c(
        c_and_a.first, boost::process::args(c_and_a.second),
        boost::process::std_in < boost::process::null,
        boost::process::std_out > out,
        boost::process::std_err > err );

    CommandResult r;
    std::thread errReader(
        [&err,&r]()
        {
            r.err.assign(
                std::istreambuf_iterator<char>(err),
                std::istreambuf_iterator<char>() );
        } );
    r.out.assign(
        std::istreambuf_iterator<char>(out),
        std::istreambuf_iterator<char>() );
    errReader.join();

    c.wait();
    r.exitCode = c.exit_code();
    return r;
}




RemoteServer::Config::Config(const boost::filesystem::path& bp, int np)
    : defaultDirectory_(bp), np_(np), originatedFromExpansion_(false)
{}

std::shared_ptr<RemoteServer::Config> RemoteServer::Config::create(rapidxml::xml_node<> *e)
{
  std::shared_ptr<RemoteServer::Config> result;

  string label = getAttribute(*e, "label");

  {
      CurrentExceptionContext ex("reading configuration of remote server %s", label.c_str());

      if (auto* ta = e->first_attribute("type"))
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
  }

  if (result)
  {
    static_cast<std::string&>(*result) = label;
  }

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

void RemoteServer::Config::save(rapidxml::xml_node<> *e, rapidxml::xml_document<> &doc) const
{
    appendAttribute(doc, *e, "label", *this );
    appendAttribute(doc, *e, "np", np_ );
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
        return std::all_of(found.begin(), found.end(), [](bool f) { return f; });
    };

    // keep the last lines for the error message
    std::deque<std::string> lastLines;
    const size_t nLastLines = 10;

    const size_t maxLines = 100*(1+pattern.size());
    size_t linesRead = 0;
    std::string line;
    while ( !allFound() && (linesRead<maxLines) && getline(is, line) )
    {
        linesRead++;

        lastLines.push_back(line);
        if (lastLines.size()>nLastLines) lastLines.pop_front();

        for (size_t i=0; i<pattern.size(); ++i)
        {
            if (!found[i])
            {
                const auto& pat = pattern[i];
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

    if (!allFound())
    {
        std::string missing;
        for (size_t i=0; i<pattern.size(); ++i)
        {
            if (!found[i])
                missing += "\n  "+pattern[i].first.str();
        }

        std::string output;
        for (const auto& l: lastLines)
            output += "\n  "+l;
        if (output.empty())
            output = "\n  (no output)";

        throw insight::Exception(
            "%s while waiting for the expected output of the remote process.\n"
            "Missing:%s\nLast lines of output:%s",
            (linesRead>=maxLines ?
                 "Too many lines of output" : "Output ended"),
            missing.c_str(), output.c_str() );
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

string RemoteServer::IPaddress() const
{
    return "127.0.0.1"; // usually through a tunnel to our local host
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
