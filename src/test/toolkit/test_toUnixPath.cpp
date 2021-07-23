#include "base/sshlinuxserver.h"

using namespace insight;

int main(int /*argc*/, char* /*argv*/[])
{
  try
  {

    std::cout<<toUnixPath("c:\\home\\hannes\\tmp")<<std::endl;
    std::cout<<toUnixPath("/home/hannes/tmp")<<std::endl;
    std::cout<<toUnixPath("hannes/tmp")<<std::endl;
    std::cout<<toUnixPath("c:hannes/tmp")<<std::endl;

  }
  catch (const std::exception& e)
  {
    std::cerr<<"Error occurred: "<<e.what()<<std::endl;
    return -1;
  }

  return 0;
}
