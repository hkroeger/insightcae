#ifndef DETECTIONHANLDER_H
#define DETECTIONHANLDER_H

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "boost/thread.hpp"

class DetectionHandler
{
  boost::asio::io_service ios_;
  boost::asio::ip::udp::endpoint listenEndpoint_;
  boost::asio::ip::udp::socket socket_;
  boost::array<char, 512> buffer_;

  std::string srvaddr_;
  int srvport_;
  const std::string& analysisName_;

  boost::thread detectionHandlerThread_;

  void waitForBroadcast();
  void receiveDone(const boost::system::error_code& /*error*/, std::size_t /*bytes_transferred*/);
  void sendDone(const boost::system::error_code& /*error*/, std::size_t /*bytes_transferred*/);

  DetectionHandler(unsigned int port,
                   const std::string& srvaddr, int srvport,
                   const std::string& analysisName);
public:

  ~DetectionHandler();

  void run();
  void stop();

  static std::unique_ptr<DetectionHandler> start(
                   unsigned int port,
                   const std::string& srvaddr, int srvport,
                   const std::string& analysisName
      );
};


#endif // DETECTIONHANLDER_H
