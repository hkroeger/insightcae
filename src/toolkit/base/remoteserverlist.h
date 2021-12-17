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

  boost::filesystem::path firstWritableLocation() const;
  void writeConfiguration(const boost::filesystem::path& file);

  iterator findServerIterator(const std::string& serverLabel ) const;
  RemoteServer::ConfigPtr findServer(const std::string& serverLabel) const;

  template<class ServerType>
  RemoteServer::ConfigPtr findFirstServerOfType(const std::string& serverLabelRegex) const
  {
    boost::regex re(serverLabelRegex);

    auto i = std::find_if(
          begin(), end(),
          [&](const value_type& entry)
          {
            return (
                bool(std::dynamic_pointer_cast<typename ServerType::Config>(entry))
                &&
                boost::regex_search(static_cast<std::string&>(*entry), re)
                );
          }
    );

    if (i==end())
      {
        throw insight::Exception(
            "No remote server of class "+std::string(typeid(ServerType).name())
            +" matching \""+serverLabelRegex+"\" found in configuration!");
      }
    return *i;
  }
};




extern RemoteServerList remoteServers;




} // namespace insight




#endif // INSIGHT_REMOTESERVERLIST_H
