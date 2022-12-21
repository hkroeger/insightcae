#include "shelltools.h"

#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;

namespace insight
{


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
