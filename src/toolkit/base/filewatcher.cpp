#include <iostream>

#include "filewatcher.h"

#include "base/externalprocess.h"

using namespace std;

namespace insight {


FileWatcher::FileWatcher(
    const boost::filesystem::path &filePath,
    std::function<void (const std::string &)> processLine,
    bool async )
{
  auto tailWorker = [filePath,processLine]() {
    try {
    auto tailjob = Job::forkExternalProcess("tail", {"-f", filePath.string()});

    tailjob->ios_run_with_interruption(
          [&](const std::string& line)
          {
            processLine(line);
          },
          [&](const std::string& line) { std::cerr<<line<<std::endl; }
    );
    }
    catch (const std::exception& ex)
    {
      std::cerr<<ex.what()<<std::endl;
    }
  };

  if (async)
  {
    tailJobThread=boost::thread(tailWorker);
  }
  else
  {
    tailWorker();
  }
}

FileWatcher::~FileWatcher()
{
  if (tailJobThread.joinable())
  {
    interrupt();
    tailJobThread.join();
  }
}

void FileWatcher::interrupt()
{
  if (tailJobThread.joinable())
    tailJobThread.interrupt();
}

} // namespace insight
