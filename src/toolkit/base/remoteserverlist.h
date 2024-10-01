#ifndef INSIGHT_REMOTESERVERLIST_H
#define INSIGHT_REMOTESERVERLIST_H

#include "base/remoteserver.h"


namespace insight {



class RemoteServerList
    : public std::set<RemoteServer::ConfigPtr>
{
  RemoteServer::ConfigPtr preferredServer_;
  std::set<RemoteServerPoolConfig> serverPools_;

public:
  RemoteServerList();

  template<class SContainer, class PContainer>
  RemoteServerList(
        const SContainer& remoteServerConfigs,
        const PContainer& remoteServerPoolConfigs,
        const std::string& preferredServerLabel
        )
  {
      reset(
          remoteServerConfigs,
          remoteServerPoolConfigs,
          preferredServerLabel );
  }

  RemoteServerList(const RemoteServerList& o);

  template<class SContainer, class PContainer>
  void reset(
      const SContainer& remoteServerConfigs,
      const PContainer& remoteServerPoolConfigs,
      const std::string& preferredServerLabel
      )
  {
      clear();
      preferredServer_.reset();
      serverPools_.clear();

      std::copy(
            remoteServerConfigs.begin(), remoteServerConfigs.end(),
            std::inserter(*this, begin())
          );

      for (auto& rspc: remoteServerPoolConfigs)
      {
          for (int id=1; id<=rspc.maxSize_; ++id)
          {
              insert(
                  rspc.configTemplate_
                      ->expanded(id) );
          }
          serverPools_.insert(rspc);
      }

      setPreferredServer(preferredServerLabel);
  }


  const std::set<RemoteServerPoolConfig>& serverPools() const;
  const RemoteServerPoolConfig& serverPool(const std::string& label) const;

  boost::filesystem::path firstWritableLocation() const;
  void writeConfiguration(const boost::filesystem::path& file);

  iterator findServerIterator(const std::string& serverLabel ) const;
  RemoteServer::ConfigPtr findServer(const std::string& serverLabel) const;

  template<class ServerType>
  RemoteServer::ConfigPtr findFirstServerOfType(const std::string& serverLabelRegex) const
  {
    if ( auto def = std::dynamic_pointer_cast<typename ServerType::Config>(preferredServer_) )
    {
        return def;
    }

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

  void setPreferredServer(const std::string& label);
  RemoteServer::ConfigPtr getPreferredServer() const;

  /**
   * @brief requestUnoccupiedServer
   * find a server with the best matching number of processors.
   * Unoccupied means the server is not running.
   * @param np
   * number of procs needed. A server with a larger number may be returned, but not with less.
   * @param poolLabel
   * if not empty, restrict to the given pool.
   * Otherwise, all known servers come into question.
   * @return
   */
  RemoteServer::ConfigPtr requestUnoccupiedServer(
      int np,
      const std::string& poolLabel = std::string()
      ) const;
};




extern RemoteServerList remoteServers;




} // namespace insight




#endif // INSIGHT_REMOTESERVERLIST_H
