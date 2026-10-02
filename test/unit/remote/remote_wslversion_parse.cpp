/*
 * Parsing of "wsl.exe --list --verbose" output (WSLLinuxServer::detectWslVersion).
 * The output is converted from UTF-16 to UTF-8 before parsing.
 */

#include "base/wsllinuxserver.h"

#include "remotetest.h"


using namespace insight;
using namespace insight::remotetest;




int main(int argc, char* argv[])
{
    TestRunner tr("remote_wslversion_parse", argc, argv);

    const std::string listOutput =
        "  NAME                   STATE           VERSION\r\n"
        "* Ubuntu-22.04           Running         2\r\n"
        "  insightcae-ubuntu      Stopped         1\r\n"
        "  Ubuntu                 Stopped         2\r\n";


    tr.run("default distribution (first line)", [&]()
    {
        int v = parseWslVersion(listOutput, "Ubuntu-22.04");
        check(v==2, "version of Ubuntu-22.04: %d", v);
    });


    tr.run("distribution in a later line", [&]()
    {
        int v = parseWslVersion(listOutput, "insightcae-ubuntu");
        check(v==1, "version of insightcae-ubuntu: %d, expected 1", v);
    });


    tr.run("distribution name must match exactly, not as substring", [&]()
    {
        // "Ubuntu" is a prefix of "Ubuntu-22.04", which is listed first
        int v = parseWslVersion(
            "  NAME            STATE           VERSION\n"
            "* Ubuntu-22.04    Running         1\n"
            "  Ubuntu          Stopped         2\n",
            "Ubuntu" );
        check(v==2, "version of Ubuntu: %d, expected 2", v);
    });


    tr.run("unknown distribution", [&]()
    {
        int v = parseWslVersion(listOutput, "Debian");
        check(v==-1, "expected -1 for an unknown distribution, got %d", v);
    });


    tr.run("no distributions installed", [&]()
    {
        int v = parseWslVersion(
            "Windows Subsystem for Linux has no installed distributions.\r\n", "Ubuntu" );
        check(v==-1, "expected -1, got %d", v);
    });


    tr.run("header only", [&]()
    {
        int v = parseWslVersion(
            "  NAME   STATE   VERSION\n", "Ubuntu" );
        check(v==-1, "expected -1, got %d", v);
    });


    return tr.finish();
}
