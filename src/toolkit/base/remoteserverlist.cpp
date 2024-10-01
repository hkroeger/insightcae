#include "remoteserverlist.h"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iterator>
#include <memory>
#include <regex>

#include "base/exception.h"
#include "base/tools.h"
#include "base/rapidxml.h"
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
                insight::dbg()<<"reading remote servers from "<<serverListFile<<std::endl;

                XMLDocument doc(serverListFile);

                auto *rootnode = doc.first_node("root");
                if (!rootnode)
                    throw insight::Exception("No valid \"remote\" node found in XML!");

                for (auto *e = rootnode->first_node(); e; e = e->next_sibling())
                {
                    if (e->name()==string("remoteServer"))
                    {
                        if ( auto rsc = RemoteServer::Config::create(e) )
                        {
                            std::string label = *rsc;

                            // replace entries, which were already existing:
                            // remove, if already present
                            auto i=findServerIterator(label);
                            if (i!=end()) erase(i);

                            servers.insert(rsc);
                        }
                        else
                        {
                            std::string label("(unlabelled)");
                            if (auto *le=e->first_attribute("label"))
                                label=std::string(le->value());
                            insight::Warning(
                                "ignored invalid remote machine configuration: %s",
                                label.c_str());
                        }
                    }
                    else if (e->name()==string("remoteServerPool"))
                    {
                        try
                        {
                            pools.insert(RemoteServerPoolConfig(e));
                        }
                        catch (insight::Exception& ex)
                        {
                            std::string label("(unlabelled)");
                            if (auto *le=e->first_attribute("label"))
                                label=std::string(le->value());
                            insight::Warning(
                                "ignored invalid remote machine pool configuration: %s (Reason: %s)",
                                label.c_str(), ex.message().c_str() );
                        }
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
        }
    }

    reset(
        servers,
        pools,
        preferredServerLabel
        );
}


RemoteServerList::RemoteServerList(const RemoteServerList& o)
  : std::set<std::shared_ptr<RemoteServer::Config> >(o),
    preferredServer_(o.preferredServer_)
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



filesystem::path RemoteServerList::firstWritableLocation() const
{
    return insight::SharedPathList::global()
            .findFirstWritableLocation( "remoteservers.list" );
}




void RemoteServerList::writeConfiguration(const boost::filesystem::path& file)
{
  using namespace rapidxml;

  if (!boost::filesystem::exists(file.parent_path()))
      boost::filesystem::create_directories(file.parent_path());

  xml_document<> doc;
  xml_node<>* decl = doc.allocate_node(node_declaration);
  decl->append_attribute(doc.allocate_attribute("version", "1.0"));
  decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
  doc.append_node(decl);
  xml_node<> *rootnode = doc.allocate_node(node_element, "root");
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

  doc.append_node(rootnode);

  ofstream f(file.string());
  f << doc;
}


RemoteServerList::iterator RemoteServerList::findServerIterator(
    const std::string& serverLabel ) const
{
  auto i = std::find_if(
        begin(), end(),
        [&](const value_type& entry)
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




RemoteServerList remoteServers;






} // namespace insight
