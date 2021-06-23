#include "remoteserverlist.h"

#include <cstdlib>
#include <regex>

#include "base/exception.h"
#include "base/tools.h"
#include "openfoam/openfoamcase.h"

#include "rapidxml/rapidxml_print.hpp"



using namespace std;
using namespace boost;




namespace insight {




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
            readFileIntoString(serverlist, content);

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
                if ( auto rsc = RemoteServer::Config::create(e) )
                {
                  insert(rsc);
                  anything_read=true;
                }
                else
                {
                  std::string label("(unlabelled)");
                  if (auto *le=e->first_attribute("label"))
                    label=std::string(le->value());
                  insight::Warning("ignored invalid remote machine configuration: "+label);
                }
              }
            }

            if (!anything_read)
              insight::Warning("Could not read valid data from "+serverlist.string());
          }
        }
    }

}


RemoteServerList::RemoteServerList(const RemoteServerList& o)
  : std::set<std::shared_ptr<RemoteServer::Config> >(o)
{
}




void RemoteServerList::writeConfiguration(const boost::filesystem::path& file)
{
  using namespace rapidxml;

  xml_document<> doc;
  xml_node<>* decl = doc.allocate_node(node_declaration);
  decl->append_attribute(doc.allocate_attribute("version", "1.0"));
  decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
  doc.append_node(decl);
  xml_node<> *rootnode = doc.allocate_node(node_element, "root");
  for (const auto& rs: *this)
  {
    xml_node<> *srvnode = doc.allocate_node(node_element, "remoteServer");
    rs->save(srvnode, doc);
    rootnode->append_node(srvnode);
  }

  doc.append_node(rootnode);

  ofstream f(file.string());
  f << doc;
}




std::shared_ptr<RemoteServer::Config> RemoteServerList::findServer(
    const std::string& serverLabel ) const
{
  auto i = std::find_if(
        begin(), end(),
        [&](const value_type& entry)
        {
          return static_cast<std::string&>(*entry)==serverLabel;
        }
  );

  if (i==end())
    {
      throw insight::Exception("Remote server \""+serverLabel+"\" not found in configuration!");
    }
  return *i;
}




RemoteServerList remoteServers;






} // namespace insight
