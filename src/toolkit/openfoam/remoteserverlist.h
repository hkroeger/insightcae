#ifndef INSIGHT_REMOTESERVERLIST_H
#define INSIGHT_REMOTESERVERLIST_H

#include "base/boost_include.h"

namespace insight {


struct RemoteServerInfo
{
  bool hasLaunchScript_;
  std::string server_;
  bfs_path defaultDir_;

  RemoteServerInfo();
  RemoteServerInfo(const std::string& server, bool hasLaunchScript, const bfs_path& defaultDir);

  bool isOnDemand() const;
};


class RemoteServerList
    : public std::map<std::string, RemoteServerInfo>
{
public:
  RemoteServerList();

#ifndef SWIG
  const RemoteServerList::value_type findServer(const std::string& server) const;
#endif
};


extern RemoteServerList remoteServers;


} // namespace insight

#endif // INSIGHT_REMOTESERVERLIST_H
