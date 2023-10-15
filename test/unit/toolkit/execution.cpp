
#include "base/softwareenvironment.h"

#include "openfoam/ofes.h"
#include "openfoam/openfoamcase.h"

using namespace insight;

int main(int /*argc*/, char*/*argv*/[])
{
  try
  {
    SoftwareEnvironment se;

    {
      std::vector<std::string> output;
      se.executeCommand("which", {"analyze"}, &output);
      if (output.size()!=1)
        throw std::runtime_error("\"which analyze\" returned no output!");
    }

    {
      std::vector<std::string> output;
      OpenFOAMCase ofc(OFEs::getCurrentOrPreferred());
      ofc.executeCommand(".", "which", {"simpleFoam"}, &output);
      if (output.size()!=1)
        throw std::runtime_error("\"which simpleFoam\" returned no output!");
    }

    {
      std::vector<std::string> output;
      OpenFOAMCase ofc(OFEs::getCurrentOrPreferred());
      bool failed=false;
      try
      {
        ofc.executeCommand("/tmp", "simpleFoam", {}, &output);
      }
      catch (std::exception& e)
      {
        std::cout<<e.what()<<std::endl;
        failed=true;
      }

      if (!failed)
        throw std::runtime_error("execution of \"simpleFoam\" in /tmp was expected to fail but succeeded!");
    }

  }
  catch (const std::exception& e)
  {
    std::cerr<<e.what()<<std::endl;
    return -1;
  }

  return 0;
}
