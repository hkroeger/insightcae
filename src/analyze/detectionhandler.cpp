#include "detectionhandler.h"

#include "base/boost_include.h"

using namespace std;
using namespace boost;

namespace ba = boost::asio;
namespace bai = boost::asio::ip;

DetectionHandler::DetectionHandler(
    ba::io_service& service, unsigned int port,
    const std::string& srvaddr, int srvport,
    const std::string& analysisName
    )
: socket_(service),
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

