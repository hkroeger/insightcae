#ifndef INSIGHT_ANALYZESERVERDETECTOR_H
#define INSIGHT_ANALYZESERVERDETECTOR_H

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "boost/thread.hpp"

namespace insight {

class AnalyzeServerDetector
{
  boost::asio::io_service ios_;
  boost::asio::ip::udp::endpoint listenEndpoint_;
  boost::asio::ip::udp::socket socket_;
  boost::array<char, 65535> buffer_;

  boost::thread serverDetectorThread_;

public:
  struct AnalyzeInstance {
    std::string serverAddress;
    int serverPort;
    std::string analysisName;
  };

  typedef std::vector<AnalyzeInstance> AnalyzeInstanceList;

protected:
  AnalyzeInstanceList detectedInstances_;

  void receiveDone(const boost::system::error_code& /*error*/, std::size_t /*bytes_transferred*/);

public:
  AnalyzeServerDetector(
      int port = 8090
      );
  ~AnalyzeServerDetector();

  static AnalyzeInstanceList detectInstances(int wait_msec, int port=8090);
};

} // namespace insight

#endif // INSIGHT_ANALYZESERVERDETECTOR_H
