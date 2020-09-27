/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

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
