#include "mountremote.h"

#include "base/exception.h"
#include "base/remoteexecution.h"
#include <cstdlib>




namespace bf = boost::filesystem;




namespace insight {




bool MountRemote::isAlreadyMounted() const
{
  std::cout<<"reading /proc/mounts"<<std::endl;
  std::ifstream f("/proc/mounts");
  std::string line;
  boost::regex e ("^([^ ]+) ([^ ]+) .*$");
  while (getline(f, line))
  {
    boost::smatch m;
    if (boost::regex_search(line, m, e))
    {
      std::string src=m[1], mp=m[2];
      std::cout<<src<<" "<<mp<<std::endl;
      if ( boost::filesystem::equivalent(mp, mountpoint_) )
      {
        std::cout<<mp<< " >> matches "<<mountpoint_<<std::endl;
        return true;
      }
    }
  }
  return false;
}




void MountRemote::mount(const std::string& server, const bfs_path& remotedir)
{
  auto gid = getgid();
  auto uid = getuid();

  std::string cmd=boost::str(boost::format( "sshfs -o uid=%d,gid=%d,follow_symlinks \"%s:%s\" \"%s\"")
                             % uid % gid
                             % server % remotedir.string()
                             % mountpoint_.string() );

  if (std::system( cmd.c_str() )!=0)
      throw insight::Exception("Could not mount remote filesystem. Failed command was: "+cmd);
}




void MountRemote::unmount()
{
  std::string cmd="fusermount -z -u \""+mountpoint_.string()+"\"";
  std::system(cmd.c_str());
}




MountRemote::MountRemote(const bfs_path& mountpoint, const std::string& server, const bfs_path& remotedir, bool keep, bool expect_mounted)
    : mountpoint_(mountpoint), keep_(keep), removeMountPoint_(false)
{
  bool is_mounted=isAlreadyMounted();

  if (is_mounted && !expect_mounted)
    throw insight::Exception("Trying to mount to directory, which is already mounted!");
  else if (!is_mounted && expect_mounted)
    throw insight::Exception("Expected mounted directory, but found it unmounted!");

  if (!expect_mounted)
    mount(server, remotedir);
}




void MountRemote::createTemporaryMountPoint()
{
  mountpoint_ = bf::unique_path( bf::temp_directory_path()/"remote-%%%%-%%%%-%%%%-%%%%" );
  try
  {
    bf::create_directories(mountpoint_);
  }
  catch (...)
  {
    throw insight::Exception("Failed to create temporary mount point \""+mountpoint_.string()+"\"!");
  }
}




MountRemote::MountRemote(const std::string &server, const bfs_path &remotedir)
  : keep_(false), removeMountPoint_(true)
{
  createTemporaryMountPoint();
  mount(server, remotedir);
}




MountRemote::MountRemote(const RemoteLocation& rloc)
  : keep_(false), removeMountPoint_(true)
{
  createTemporaryMountPoint();
  mount(rloc.server(), rloc.remoteDir());
}




MountRemote::~MountRemote()
{
  if (!keep_)
  {
    unmount();
    if (removeMountPoint_)
    {
      bf::remove(mountpoint_);
    }
  }
}




const boost::filesystem::path& MountRemote::mountpoint() const
{
  return mountpoint_;
}




} // namespace insight
