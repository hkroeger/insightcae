#include "remoteserverlist.h"

#include <cstdlib>
#include <regex>

#include "base/exception.h"
#include "base/tools.h"
#include "openfoam/openfoamcase.h"

using namespace std;
using namespace boost;

namespace insight {


RemoteServerInfo::RemoteServerInfo()
{}


RemoteServerInfo::RemoteServerInfo(const std::string& serverName, const bfs_path& defaultDir)
  : serverName_(serverName), defaultDir_(defaultDir)
{}




RemoteServerList::RemoteServerList()
{
  SharedPathList paths;
  for ( const bfs_path& p: paths )
  {
      if ( exists(p) && is_directory ( p ) )
      {
          bfs_path serverlist = bfs_path(p) / "remoteservers.list";

          if ( exists(serverlist) )
          {
              std::string line;
              std::ifstream f(serverlist.c_str());
              bool anything_read=false;
              int i=0;
              while (getline(f, line))
                {
                  i++;

                  std::regex pat("([^ ]+) *= *([^ ]+):([^ ]+)");
                  std::smatch m;
                  std::regex_match(line, m, pat);
                  if (m.size()!=4)
                    {
                      insight::Warning(boost::str(
                               boost::format("invalid remote server config in line %d of file %s (content: \"%s\"). Ignored")
                                                 % i % serverlist.string() % line
                                                 ));
                    }
                  else
                    {
                      RemoteServerInfo s;
                      std::string key= m[1];
                      s.serverName_ = m[2];
                      s.defaultDir_ = bfs_path(m[3]);
                      (*this)[key]=s;
                      anything_read=true;
                    }
                }

              if (!anything_read)
                insight::Warning("Could not read valid data from "+serverlist.string());
            }
        }
    }
}


const RemoteServerList::value_type RemoteServerList::findServer(const std::string& server) const
{
  auto i = find(server);
  if (i==end())
    {
      throw insight::Exception("Remote server \""+server+"\" not found in configuration!");
    }
  return *i;
}


RemoteServerList remoteServers;



} // namespace insight
