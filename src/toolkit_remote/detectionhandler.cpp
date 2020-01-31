#include "detectionhandler.h"

#include "base/boost_include.h"

using namespace std;
using namespace boost;

namespace ba = boost::asio;
namespace bai = boost::asio::ip;

DetectionHandler::DetectionHandler(
    unsigned int port,
    const std::string& srvaddr, int srvport,
    const std::string& analysisName
    )
: socket_(ios_),
  srvaddr_(srvaddr), srvport_(srvport),
  analysisName_(analysisName)
{
  socket_.open(bai::udp::v4());
  socket_.set_option(bai::udp::socket::reuse_address(true));
  socket_.set_option(ba::socket_base::broadcast(true));
  listenEndpoint_=bai::udp::endpoint(
        bai::address_v4::broadcast(), port
        );
  socket_.bind(listenEndpoint_);
  waitForBroadcast();

  detectionHandlerThread_ =
    boost::thread(
        [&]()
        {
          try
          {
            run();
          }
          catch (std::exception& e)
          {
            cerr<<"Error: could not start broadcast listener! Reason: "<<e.what()<<endl;
            cerr<<"Note: This execution server detection will not be detectable."<<endl;
          }
        }
  );
}

DetectionHandler::~DetectionHandler()
{
  stop();
}

void DetectionHandler::run()
{
  ios_.run();
}

void DetectionHandler::stop()
{
  ios_.stop();
}

std::unique_ptr<DetectionHandler> DetectionHandler::start
(
    unsigned int port,
    const string &srvaddr, int srvport,
    const string &analysisName
)
{
  std::unique_ptr<DetectionHandler> dh
      (
        new DetectionHandler
        (
          port,
          srvaddr, srvport,
          analysisName
        )
      );

  return dh;
}

void DetectionHandler::waitForBroadcast()
{
    socket_.async_receive_from
        (
          ba::buffer(buffer_), listenEndpoint_,
          bind(
            &DetectionHandler::receiveDone, this,
            ba::placeholders::error,
            ba::placeholders::bytes_transferred
          )
        );

}

void DetectionHandler::receiveDone(const boost::system::error_code& /*error*/, std::size_t /*bytes_transferred*/)
{
    std::cout << "Received detection broadcast " << std::endl;

    std::string response = srvaddr_+":"+lexical_cast<string>(srvport_)+" \""+analysisName_+"\"";

    socket_.async_send_to
        (
          ba::buffer(response), listenEndpoint_,
          bind(
            &DetectionHandler::sendDone, this,
            ba::placeholders::error,
            ba::placeholders::bytes_transferred
          )
        );
}

void DetectionHandler::sendDone(const boost::system::error_code& /*error*/, std::size_t /*bytes_transferred*/)
{
    waitForBroadcast();
}

