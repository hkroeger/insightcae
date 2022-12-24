#ifndef SHELLTOOLS_H
#define SHELLTOOLS_H

#include <string>
#include <vector>


namespace insight
{

char* tocbuf(const std::string& line);


std::string collectIntoSingleCommand(
        const std::string& cmd,
        const std::vector<std::string>& args = std::vector<std::string>() );

std::string escapeShellSymbols(const std::string& expr);


}

#endif // SHELLTOOLS_H
