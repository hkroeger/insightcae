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


char * joblistdump_torun(char *command)
{
    std::string cmd(command);
    return insight::tocbuf( "tsp "+cmd+"\n" );
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

    return insight::tocbuf(lines);
}
