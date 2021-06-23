#ifndef INSIGHT_REMOTESERVERLIST_H
#define INSIGHT_REMOTESERVERLIST_H

#include "base/remoteserver.h"


namespace insight {



class RemoteServerList
    : public std::set<RemoteServer::ConfigPtr>
{
public:
  RemoteServerList();
  RemoteServerList(const RemoteServerList& o);

  void writeConfiguration(const boost::filesystem::path& file);

  RemoteServer::ConfigPtr findServer(const std::string& serverLabel) const;
};




extern RemoteServerList remoteServers;




} // namespace insight




#endif // INSIGHT_REMOTESERVERLIST_H
