/*
 * Precondition check for all tests, which use the remote server "localhost"
 * (CTest fixture "remote_ssh").
 *
 * Expected setup:
 *  - an entry labelled "localhost" of type SSHLinux in remoteservers.list
 *    (e.g. ~/.insight/share/remoteservers.list), like
 *
 *    <remoteServer label="localhost" type="SSHLinux" host="localhost" baseDirectory="/tmp"/>
 *
 *  - passwordless ssh to that host
 *  - analyze, rsync and isPVFindPort.sh in the PATH of non-interactive ssh sessions
 *
 * If this check fails, all dependent tests are reported as "Not Run".
 */

#include <iostream>

#include "backends.h"


using namespace insight::remotetest;


int main(int, char*[])
{
    auto be = sshLocalhostBackend();
    auto reason = be->checkAvailability();
    if (!reason.empty())
    {
        std::cerr
            << "The remote server \"localhost\" is not usable for testing:\n  "
            << reason << "\n\n"
            << "Please configure a remote server labelled \"localhost\" (type SSHLinux)\n"
            << "with passwordless ssh access, e.g. in ~/.insight/share/remoteservers.list:\n"
            << "  <root>\n"
            << "    <remoteServer label=\"localhost\" type=\"SSHLinux\" host=\"localhost\" baseDirectory=\"/tmp\"/>\n"
            << "  </root>\n"
            << "The tests depending on it will not be run." << std::endl;
        return 1;
    }

    std::cout<<"remote server \"localhost\" is usable."<<std::endl;
    return 0;
}
