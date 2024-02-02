#include "build_command_string.h"


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>

extern "C" {
#include "main.h"
}

#include "../toolkit/base/shelltools.h"


char *build_command_string()
{
    int num;
    char **array;

    num = command_line.command.num;
    array = command_line.command.array;

    std::string commandstring(array[0]);

    /* Build the command */
    for (int i = 1; i < num; ++i)
    {
        commandstring += " \"" + insight::escapeShellSymbols(array[i]) + "\"";
    }

    return insight::tocbuf(commandstring);
}
