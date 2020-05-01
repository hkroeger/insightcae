#ifndef INSIGHT_MOUNTREMOTE_H
#define INSIGHT_MOUNTREMOTE_H

#include "base/boost_include.h"
#include "base/remotelocation.h"
#include "base/remoteserverlist.h"


namespace insight {

class MountRemote
{
    bfs_path mountpoint_;
    bool keep_;
    bool removeMountPoint_;

    bool isAlreadyMounted() const;
    void mount(const std::string& server, const bfs_path& remotedir);
    void unmount();
    void createTemporaryMountPoint();

public:
    /**
     * @brief MountRemote
     * Mount remote directory via sshfs to existing local directory
     * @param mountpoint
     * @param server
     * @param remotedir
     * @param keep
     * @param expect_mounted
     */
    MountRemote(const bfs_path& mountpoint, const std::string& server, const bfs_path& remotedir, bool keep=false, bool expect_mounted=false);

    /**
    * @brief MountRemote
    * Mount remote directory via sshfs to temporary directory
    * @param mountpoint
    * @param server
    * @param remotedir
    * @param keep
    * @param expect_mounted
    */
   MountRemote(const std::string& server, const bfs_path& remotedir);

   MountRemote(const RemoteLocation& rloc);

    ~MountRemote();

   const boost::filesystem::path& mountpoint() const;
};

} // namespace insight

#endif // INSIGHT_MOUNTREMOTE_H
