#include "joblistdump_torun.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/msg.h>

#include <string>
#include <iostream>
#include "../toolkit/base/shelltools.h"

#include "boost/filesystem.hpp"

extern "C" void error(const char *str, ...);


char* tocbuf(const std::string& line)
{
    char *cline = (char *) malloc(line.size()+1);
    if (cline == nullptr)
        error("Malloc for %i failed.\n", line.size()+1);
    line.copy(cline, line.size());
    cline[line.size()]='\0';

    return cline;
}



char * joblistdump_torun(char *command)
{
    std::string cmd(command);

    return tocbuf(
                "tsp \""+insight::escapeShellSymbols(cmd)+"\"\n"
                );
}


char * joblistdump_envvars()
{
    std::string lines;
    for (std::string fvar: {"TS_SOCKET", "TS_SAVELIST"})
    {
        auto absf = boost::filesystem::absolute( getenv(fvar.c_str()) );
        lines += "export "+fvar+"=\""+absf.string()+"\"\n";
    }
    for (std::string var: {
         "TS_MAILTO", "TS_MAXFINISHED", "TS_MAXCONN", "TS_SLOTS" })
    {
        if (auto val = getenv(var.c_str()))
        {
            lines += "export "+var+"="+ val +"\n";
        }
    }

    return tocbuf(lines);
}
