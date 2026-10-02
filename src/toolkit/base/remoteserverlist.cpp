#include "remoteserverlist.h"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iterator>
#include <memory>
#include <regex>

#include "base/exception.h"
#include "base/warningdispatcher.h"
#include "base/tools.h"
#include "base/rapidxml.h"
#include "base/translations.h"
#include "boost/algorithm/string/predicate.hpp"
#include "openfoam/openfoamcase.h"

#include "rapidxml/rapidxml_print.hpp"

#include <boost/range/adaptor/reversed.hpp>
#include <utility>

using namespace std;
using namespace boost;




namespace insight {




RemoteServerList::RemoteServerList()
{
    std::set<RemoteServer::ConfigPtr> servers;
    std::set<RemoteServerPoolConfig> pools;
    std::string preferredServerLabel;

    auto paths = SharedPathList::global();
    for ( const bfs_path& p: boost::adaptors::reverse(paths) ) // reverse: start with global, then per-user to possibly overwrite global
    {
        if ( exists(p) && is_directory ( p ) )
        {
            bfs_path serverListFile = bfs_path(p) / "remoteservers.list";

            if ( exists(serverListFile) )
            {
                try
                {
                    CurrentExceptionContext ex(
                            _("reading remote servers from %s"),
                            serverListFile.c_str() );

                    XMLDocument doc(serverListFile);

                    auto *rootnode = doc.first_node("root");
                    if (!rootnode)
                    {
                        throw insight::Exception(
                            _("No valid 'root' node found in XML!")
                            );
                    }

                    // labels defined in this file: duplicates within one file are ignored
                    std::set<std::string> labelsInThisFile;
                    auto isDuplicateInThisFile = [&](const std::string& label)
                    {
                        if (!labelsInThisFile.insert(label).second)
                        {
                            insight::Warning(
                                _("remote server %s is defined more than once in %s. Only the first definition is used."),
                                label.c_str(), serverListFile.c_str() );
                            return true;
                        }
                        return false;
                    };

                    for (auto *e = rootnode->first_node(); e; e = e->next_sibling())
                    {
                        try
                        {
                            if (e->name()==string("remoteServer"))
                            {
                                if ( auto rsc = RemoteServer::Config::create(e) )
                                {
                                    std::string label = *rsc;

                                    if (!isDuplicateInThisFile(label))
                                    {
                                        // replace entries from previously read files (e.g. global config)
                                        for (auto i=servers.begin(); i!=servers.end(); )
                                        {
                                            if (static_cast<const std::string&>(**i)==label)
                                                i=servers.erase(i);
                                            else
                                                ++i;
                                        }
                                        servers.insert(rsc);
                                    }
                                }
                                else
                                {
                                    std::string label("(unlabelled)");
                                    if (auto *le=e->first_attribute("label"))
                                        label=std::string(le->value());
                                    insight::Warning(
                                        _("ignored invalid remote machine configuration: %s"),
                                        label.c_str());
                                }
                            }
                            else if (e->name()==string("remoteServerPool"))
                            {
                                try
                                {
                                    RemoteServerPoolConfig pool(e);
                                    std::string label = *pool.configTemplate_;

                                    if (!isDuplicateInThisFile(label))
                                    {
                                        // replace pools from previously read files
                                        for (auto i=pools.begin(); i!=pools.end(); )
                                        {
                                            if (static_cast<const std::string&>(*i->configTemplate_)==label)
                                                i=pools.erase(i);
                                            else
                                                ++i;
                                        }
                                        pools.insert(pool);
                                    }
                                }
                                catch (insight::Exception& ex)
                                {
                                    std::string label("(unlabelled)");
                                    if (auto *le=e->first_attribute("label"))
                                        label=std::string(le->value());
                                    insight::Warning(
                                        _("ignored invalid remote machine pool configuration: %s (Reason: %s)"),
                                        label.c_str(), ex.message().c_str() );
                                }
                            }
                        }
                        catch (insight::Exception& ex)
                        {
                            insight::Warning(ex);
                        }
                    }

                    if (auto *prevSrvNode = rootnode->first_node("preferredServer"))
                    {
                        if (auto *lbl = prevSrvNode->first_attribute("label"))
                        {
                            insight::dbg()<<"setting preferred server as "<<lbl->value()<<std::endl;
                            preferredServerLabel = lbl->value();
                        }
                    }
                }
                catch (std::exception& ex)
                {
                    // a broken file must not prevent reading the other configuration files
                    insight::Warning(
                        _("ignoring remote server list %s: %s"),
                        serverListFile.c_str(), ex.what() );
                }
            }
        }
    }

    reset(
        servers,
        pools,
        preferredServerLabel
        );
}


RemoteServerList::RemoteServerList(const RemoteServerList& o)
  : std::set<RemoteServer::ConfigPtr>(o),
    preferredServer_(o.preferredServer_),
    serverPools_(o.serverPools_)
{
}

const std::set<RemoteServerPoolConfig> &RemoteServerList::serverPools() const
{
    return serverPools_;
}

const RemoteServerPoolConfig &RemoteServerList::serverPool(const std::string &label) const
{
    auto i = std::find_if(
        serverPools_.begin(), serverPools_.end(),
        [&label](const RemoteServerPoolConfig& c)
        { return *c.configTemplate_ == label; }
        );
    insight::assertion(
        i!=serverPools_.end(),
        "there is no server pool labelled %s", label.c_str() );
    return *i;
}



boost::filesystem::path RemoteServerList::firstWritableLocation() const
{
    return insight::SharedPathList::global()
            .findFirstWritableLocation( "remoteservers.list" );
}




void RemoteServerList::writeConfiguration(const boost::filesystem::path& file)
{
  using namespace rapidxml;

  if (!boost::filesystem::exists(file.parent_path()))
      boost::filesystem::create_directories(file.parent_path());

  XMLDocument doc;
  xml_node<> *rootnode = doc.rootNode;
  for (const auto& rs: *this)
  {
      if (!rs->wasExpanded())
      {
        xml_node<> *srvnode = doc.allocate_node(node_element, "remoteServer");
        rs->save(srvnode, doc);
        rootnode->append_node(srvnode);
      }
  }
  for (const auto& rspc: serverPools_)
  {
      xml_node<> *srvnode = doc.allocate_node(node_element, "remoteServerPool");
      rspc.save(srvnode, doc);
      rootnode->append_node(srvnode);
  }
  if (auto ps = getPreferredServer())
  {
    xml_node<> *prefSrvNode = doc.allocate_node(node_element, "preferredServer");
    prefSrvNode->append_attribute(
                doc.allocate_attribute("label",
                  doc.allocate_string( (*ps).c_str() )));
    rootnode->append_node(prefSrvNode);
  }

  doc.saveToFile(file);
}


RemoteServerList::iterator RemoteServerList::findServerIterator(
    const std::string& serverLabel ) const
{
  auto i = std::find_if(
        begin(), end(),
        [&](auto& entry)
        {
          return static_cast<std::string&>(*entry)==serverLabel;
        }
  );

  return i;
}


std::shared_ptr<RemoteServer::Config> RemoteServerList::findServer(
    const std::string& serverLabel ) const
{
  auto i = findServerIterator(serverLabel);
  if (i==end())
    {
      throw insight::Exception("Remote server \""+serverLabel+"\" not found in configuration!");
    }
  return *i;
}



void RemoteServerList::setPreferredServer(const std::string &label)
{
    if (label.empty())
    {
        preferredServer_.reset();
    }
    else
    {
        preferredServer_ = findServer(label);
    }
}



RemoteServer::ConfigPtr RemoteServerList::getPreferredServer() const
{
    return preferredServer_;
}


RemoteServer::ConfigPtr RemoteServerList::requestUnoccupiedServer(int np, const std::string &poolLabel) const
{
    // candidates for occupation test
    std::vector<RemoteServer::ConfigPtr> candidates;
    if (!poolLabel.empty())
    {
        std::copy_if(
            begin(), end(),
            std::back_inserter(candidates),
            [&](const RemoteServer::ConfigPtr& srv)
            {
                return boost::starts_with(*srv, poolLabel+"/");
            }
        );
    }
    else
    {
        std::copy(
            begin(), end(),
            std::back_inserter(candidates)
        );
    }

    // move preferred server to first position
    if (preferredServer_)
    {
        auto ips=std::find(
            candidates.begin(), candidates.end(),
            preferredServer_ );
        if (ips!=candidates.end())
        {
            if (ips!=candidates.begin())
            {
                std::swap(*ips, candidates.front());
            }
        }
    }

    std::multimap<int, RemoteServer::ConfigPtr> orderedCandidates;

    for (auto& sc: candidates)
    {
        if (sc->np_>=np)
        {
            auto leftover=sc->unoccupiedProcessors()-np;
            if (leftover>=0)
            {
                orderedCandidates.insert({leftover, sc});
            }
        }
    }

    insight::dbg()<<"= CANDIDATES =\n";
    for (auto &c: orderedCandidates)
    {
        insight::dbg()<<*c.second<<" (leaves "<<c.first<<" procs unused)\n";
    }
    insight::dbg()<<"==============\n";

    if (orderedCandidates.size())
        return orderedCandidates.begin()->second;

    return nullptr;
}



RemoteServerList& remoteServers()
{
    static RemoteServerList theRemoteServers;
    return theRemoteServers;
}






} // namespace insight
