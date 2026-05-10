#ifndef INSIGHT_FILEWATCHER_H
#define INSIGHT_FILEWATCHER_H

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

namespace insight {

class FileWatcher
{
  boost::thread tailJobThread;

public:
  FileWatcher(
      const boost::filesystem::path& filePath,
      std::function<void(const std::string&)> processLine,
      bool async=true );

  ~FileWatcher();

  void interrupt();
};

} // namespace insight

#endif // INSIGHT_FILEWATCHER_H
