#include "shelltools.h"

#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;

namespace insight
{


char* tocbuf(const std::string& line)
{
    char *buffer = (char *) malloc(line.size()+1);
    if (buffer == nullptr)
        throw std::runtime_error("Malloc failed.");
    auto l = line.copy(buffer, line.size());
    buffer[l]='\0';

    return buffer;
}

std::string collectIntoSingleCommand( const std::string& cmd, const std::vector<std::string>& args )
{
  std::string res = cmd;
  for (const auto& arg: args)
  {
    res += " \""+arg+"\""; // leading space!
  }
  return res;
}

string escapeShellSymbols(const string &expr)
{
  string res(expr);
  algorithm::replace_all(res, "\\", "\\\\");
  algorithm::replace_all(res, "$", "\\$");
  algorithm::replace_all(res, "\"", "\\\"");
  return res;
}

}
