#include "localshellserver.h"

#include <fnmatch.h>

#include "base/exception.h"
#include "base/rapidxml.h"

#include "rapidxml/rapidxml_print.hpp"

namespace fs = boost::filesystem;


namespace insight {
namespace remotetest {




LocalShellServer::Config::Config(
    const boost::filesystem::path& baseDirectory,
    int np,
    const std::string& label )
    : LinuxRemoteServer::Config(baseDirectory, np)
{
    static_cast<std::string&>(*this)=label;
}




std::shared_ptr<RemoteServer> LocalShellServer::Config::instance() const
{
    return std::make_shared<LocalShellServer>(*this);
}




std::pair<boost::filesystem::path,std::vector<std::string> >
LocalShellServer::Config::commandAndArgs(const std::string& command) const
{
    return { "/bin/bash", { "-c", command } };
}




bool LocalShellServer::Config::isDynamicallyAllocated() const
{
    return false;
}




void LocalShellServer::Config::save(rapidxml::xml_node<> *e, rapidxml::xml_document<>& doc) const
{
    appendAttribute(doc, *e, "label", *this );
    appendAttribute(doc, *e, "type", "LocalShell" );
    appendAttribute(doc, *e, "baseDirectory", defaultDirectory_.string() );
}




RemoteServer::ConfigPtr LocalShellServer::Config::clone() const
{
    return std::make_shared<Config>(*this);
}




bool LocalShellServer::Config::isDynamicallyCreatable() const
{
    return false;
}




bool LocalShellServer::Config::isDynamicallyDestructable() const
{
    return false;
}




LocalShellServer::LocalShellServer(const Config& cfg)
    : serverConfig_(cfg)
{}




const RemoteServer::Config& LocalShellServer::config() const
{
    return serverConfig_;
}




static bool isExcluded(
    const fs::path& relPath,
    bool includeProcessorDirectories,
    const std::vector<std::string>& exclude_pattern )
{
    std::vector<std::string> pats(exclude_pattern);
    if (!includeProcessorDirectories)
        pats.push_back("processor*");

    for (const auto& part: relPath)
    {
        for (const auto& p: pats)
        {
            if (fnmatch(p.c_str(), part.string().c_str(), 0)==0)
                return true;
        }
    }
    return false;
}




static void copyTree(
    const fs::path& from,
    const fs::path& to,
    bool includeProcessorDirectories,
    const std::vector<std::string>& exclude_pattern )
{
    insight::assertion(
        fs::is_directory(from),
        "source directory %s does not exist", from.string().c_str() );

    fs::create_directories(to);
    for (fs::recursive_directory_iterator i(from), e; i!=e; ++i)
    {
        auto rel = fs::relative(i->path(), from);
        if (isExcluded(rel, includeProcessorDirectories, exclude_pattern))
        {
            if (fs::is_directory(i->path()))
                i.no_push();
            continue;
        }
        if (fs::is_directory(i->path()))
            fs::create_directories(to/rel);
        else
            fs::copy_file(i->path(), to/rel, fs::copy_option::overwrite_if_exists);
    }
}




void LocalShellServer::putFile(
    const boost::filesystem::path& localFilePath,
    const boost::filesystem::path& remoteFilePath,
    std::function<void(int,const std::string&)> )
{
    fs::copy_file(localFilePath, remoteFilePath, fs::copy_option::overwrite_if_exists);
}




void LocalShellServer::syncToRemote(
    const boost::filesystem::path& localDir,
    const boost::filesystem::path& remoteDir,
    bool includeProcessorDirectories,
    const std::vector<std::string>& exclude_pattern,
    std::function<void(int,const std::string&)> )
{
    copyTree(localDir, remoteDir, includeProcessorDirectories, exclude_pattern);
}




void LocalShellServer::syncToLocal(
    const boost::filesystem::path& localDir,
    const boost::filesystem::path& remoteDir,
    bool includeProcessorDirectories,
    const std::vector<std::string>& exclude_pattern,
    std::function<void(int,const std::string&)> )
{
    copyTree(remoteDir, localDir, includeProcessorDirectories, exclude_pattern);
}




} // namespace remotetest
} // namespace insight
