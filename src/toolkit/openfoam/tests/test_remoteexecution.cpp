
#include "base/boost_include.h"
#include "openfoam/remoteexecution.h"

using namespace std;
using namespace boost;
using namespace insight;

namespace fs=boost::filesystem;

int main()
{
  auto d = fs::unique_path( fs::temp_directory_path() / "remoteexec-test-%%%%%%" );
  fs::create_directory(d);
  int ret=0;

  try
  {
    {
      ofstream f( (d/"meta.foam").c_str() );
      cout << "localhost:"<<d.string()<<endl;
      f << "localhost:"<<d.string()<<endl;
    }

    RemoteExecutionConfig ec(d);

    try
    {
      ec.queueRemoteCommand("ls -l");
      ec.waitLastCommandFinished();

      ec.queueRemoteCommand("echo $SSH_CLIENT");
      ec.waitLastCommandFinished();
    }
    catch (...)
    {
      ret=-2;
    }

    ec.cancelRemoteCommands();
  }
  catch (const std::exception& e)
  {
   cerr<<e.what();
   ret=-1;
  }

  fs::remove_all(d);
  return ret;
}
