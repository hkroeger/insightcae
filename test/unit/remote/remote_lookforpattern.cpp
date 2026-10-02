/*
 * RemoteServer::lookForPattern scans the output of a freshly launched
 * background process (e.g. for its PID) before the process is detached.
 */

#include <sstream>

#include "base/remoteserver.h"

#include "remotetest.h"


using namespace insight;
using namespace insight::remotetest;




int main(int argc, char* argv[])
{
    TestRunner tr("remote_lookforpattern", argc, argv);


    tr.run("single pattern is found and captured", [&]()
    {
        std::istringstream is(
            "some output\n"
            "PID===4711===PID\n"
            "more output\n" );

        std::vector<std::string> m;
        RemoteServer::lookForPattern(
            is, { { boost::regex("PID===([0-9]+)===PID"), &m } } );

        check(m.size()==2, "expected full match and one capture, got %d entries", int(m.size()));
        check(m[1]=="4711", "captured PID: "+m[1]);
    });


    tr.run("multiple patterns in arbitrary order", [&]()
    {
        std::istringstream is(
            "READY on port 8123\n"
            "noise\n"
            "PID===42===PID\n" );

        std::vector<std::string> pid, port;
        RemoteServer::lookForPattern(
            is,
            {
                { boost::regex("PID===([0-9]+)===PID"), &pid },
                { boost::regex("READY on port ([0-9]+)"), &port }
            } );

        check(pid.size()==2 && pid[1]=="42", "PID not captured");
        check(port.size()==2 && port[1]=="8123", "port not captured");
    });


    tr.run("pattern without capture target", [&]()
    {
        std::istringstream is("started\n");
        RemoteServer::lookForPattern(
            is, { { boost::regex("started"), nullptr } } );
    });


    tr.run("end of output without the pattern throws instead of hanging", [&]()
    {
        // e.g.: ssh fails to connect, the remote command is not found,
        // or the remote shell exits before printing the PID
        auto is = std::make_shared<std::istringstream>(
            "ssh: connect to host example port 22: Connection refused\n" );

        checkCompletesWithin(
            std::chrono::seconds(2),
            [is]()
            {
                std::vector<std::string> m;
                expectThrows<insight::Exception>(
                    [&]()
                    {
                        RemoteServer::lookForPattern(
                            *is, { { boost::regex("PID===([0-9]+)===PID"), &m } } );
                    },
                    "looking for a pattern in output, which ends without it" );
            },
            "lookForPattern on output without the pattern" );
    });


    tr.run("empty output throws instead of hanging", [&]()
    {
        auto is = std::make_shared<std::istringstream>("");

        checkCompletesWithin(
            std::chrono::seconds(2),
            [is]()
            {
                std::vector<std::string> m;
                expectThrows<insight::Exception>(
                    [&]()
                    {
                        RemoteServer::lookForPattern(
                            *is, { { boost::regex("PID===([0-9]+)===PID"), &m } } );
                    },
                    "looking for a pattern in empty output" );
            },
            "lookForPattern on empty output" );
    });


    tr.run("too much output without the pattern throws", [&]()
    {
        std::string out;
        for (int i=0; i<10000; ++i) out+="line "+std::to_string(i)+"\n";
        auto is = std::make_shared<std::istringstream>(out);

        checkCompletesWithin(
            std::chrono::seconds(5),
            [is]()
            {
                std::vector<std::string> m;
                expectThrows<insight::Exception>(
                    [&]()
                    {
                        RemoteServer::lookForPattern(
                            *is, { { boost::regex("PID===([0-9]+)===PID"), &m } } );
                    },
                    "looking for a pattern in long output without it" );
            },
            "lookForPattern on long output" );
    });


    // abandoned (hanging) threads may still be running: exit immediately
    tr.exit();
}
