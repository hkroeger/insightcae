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


RemoteServerInfo::RemoteServerInfo(const std::string& server, bool hasLaunchScript, const bfs_path& defaultDir)
  : hasLaunchScript_(hasLaunchScript), server_(server), defaultDir_(defaultDir)
{}

bool RemoteServerInfo::isOnDemand() const
{
  return hasLaunchScript_;
}


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
            bool anything_read=false;

            // read xml
            string content;
            try
            {
                std::ifstream in(serverlist.c_str());
                istreambuf_iterator<char> fbegin(in), fend;
                std::copy(fbegin, fend, back_inserter(content));
            }
            catch (...)
            {
                throw insight::Exception("Failed to read file "+serverlist.string());
            }

            using namespace rapidxml;
            xml_document<> doc;

            try {
              doc.parse<0>(&content[0]);
            }
            catch (...)
            {
              throw insight::Exception("Failed to parse XML from file "+serverlist.string());
            }

            auto *rootnode = doc.first_node("root");
            if (!rootnode)
              throw insight::Exception("No valid \"remote\" node found in XML!");

            for (xml_node<> *e = rootnode->first_node(); e; e = e->next_sibling())
            {
              if (e->name()==string("remoteServer"))
              {
                RemoteServerInfo s;

                string label(e->first_attribute("label")->value());
                s.defaultDir_ = boost::filesystem::path(e->first_attribute("baseDirectory")->value());

                auto* ha = e->first_attribute("host");
                auto* lsaa = e->first_attribute("launchScript");
                if (ha && lsaa)
                {
                  throw insight::Exception("Invalid configuration of remote server "+label+": either host name or launch script must be specified!");
                }
                else if (ha)
                {
                  s.server_=ha->value();
                  s.hasLaunchScript_=false;
                  (*this)[label]=s;
                  anything_read=true;
                }
                else if (lsaa)
                {
                  s.server_=lsaa->value();
                  s.hasLaunchScript_=true;
                  (*this)[label]=s;
                  anything_read=true;
                }
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
