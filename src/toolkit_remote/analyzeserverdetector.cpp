#include "analyzeserverdetector.h"

#include "base/boost_include.h"

using namespace std;
using namespace boost;

namespace ba = boost::asio;
namespace bai = boost::asio::ip;


namespace insight {


void AnalyzeServerDetector::receiveDone(const system::error_code &, size_t)
{
  std::string response(buffer_.data());
  cout << "RECV: "<<response << endl;

  bool ok=false;

  size_t i = response.find(' ');
  if (i!=std::string::npos)
  {
    std::string addr=response.substr(0,i);
    std::string name=response.substr(i+1);

    size_t j=addr.find(':');
    if (j!=std::string::npos)
    {
      if (name.size()>=2)
      {
        if ((name[0]=='"') && (name[name.size()-1]=='"'))
        {
          AnalyzeInstance ai;
          ai.serverAddress=addr.substr(0, j);
          ai.serverPort=lexical_cast<int>(addr.substr(j+1));
          ai.analysisName=boost::algorithm::trim_copy_if(name, boost::is_any_of("\"") );
          detectedInstances_.push_back(ai);
          ok=true;
        }
      }
    }
  }

  if (!ok)
  {
    cerr<<"Warning: got malformed response: >>"<<response<<"<<, ignored!"<<endl;
  }



  socket_.async_receive_from
      (
        ba::buffer(buffer_), listenEndpoint_,
        bind(
          &AnalyzeServerDetector::receiveDone, this,
          ba::placeholders::error,
          ba::placeholders::bytes_transferred
        )
      );
}



AnalyzeServerDetector::AnalyzeServerDetector(
    int port
    )
  : socket_(ios_)
{
  socket_.open(bai::udp::v4());
  socket_.set_option(bai::udp::socket::reuse_address(true));
  socket_.set_option(ba::socket_base::broadcast(true));
  bai::udp::endpoint sendEndpoint(
        bai::address_v4::broadcast(), port
        );

  std::string msg = "?";

  socket_.send_to(ba::buffer(msg), sendEndpoint);


  socket_.async_receive_from
      (
        ba::buffer(buffer_), listenEndpoint_,
        bind(
          &AnalyzeServerDetector::receiveDone, this,
          ba::placeholders::error,
          ba::placeholders::bytes_transferred
        )
      );

  serverDetectorThread_ =
    boost::thread(
        [&]()
        {
          try
          {
            ios_.run();
          }
          catch (std::exception& e)
          {
            cerr<<"Error: could not start broadcast service! Reason: "<<e.what()<<endl;
          }
        }
  );
}

AnalyzeServerDetector::~AnalyzeServerDetector()
{
  ios_.stop();
  serverDetectorThread_.join();
}

AnalyzeServerDetector::AnalyzeInstanceList AnalyzeServerDetector::detectInstances(int wait_msec, int port)
{
  AnalyzeServerDetector asd(port);
  boost::this_thread::sleep_for(boost::chrono::milliseconds(wait_msec));
  return asd.detectedInstances_;
}

} // namespace insight
