#include "taskspoolerinterface.h"
#include "base/exception.h"
#include "base/tools.h"
#include <boost/asio.hpp>
#include <boost/process/async.hpp>

using namespace std;
using namespace boost;

namespace insight
{



bool TaskSpoolerInterface::JobList::hasRunningJobs() const
{
  for (const auto& j: *this)
  {
    if (j.state==Running) return true;
  }
  return false;
}


bool TaskSpoolerInterface::JobList::hasQueuedJobs() const
{
  for (const auto& j: *this)
  {
    if (j.state==Queued) return true;
  }
  return false;
}


bool TaskSpoolerInterface::JobList::hasFailedJobs() const
{
  for (const auto& j: *this)
  {
    if ((j.state==Finished) && (j.elevel!=0)) return true;
  }
  return false;
}

TaskSpoolerInterface::TaskSpoolerInterface(const boost::filesystem::path& socket, RemoteServerPtr server)
  : server_(server),
    socket_(socket),
    env_(boost::this_process::environment())
{
  if (!server_)
    env_["TS_SOCKET"]=socket.string();
}

TaskSpoolerInterface::~TaskSpoolerInterface()
{
  stopTail();

  auto jl=jobs();
  if (! (jl.hasRunningJobs() || jl.hasQueuedJobs()) )
  {
    stopTaskspoolerServer();
  }
}


TaskSpoolerInterface::JobList TaskSpoolerInterface::jobs() const
{
  JobList jl;
  boost::process::ipstream is;
  std::shared_ptr<boost::process::child> c;


  if (server_)
  {
    std::ostringstream cmd;
    cmd << "TS_SOCKET="<<socket_.string()<<" tsp";

    c = server_->launchCommand(
              cmd.str(),
              boost::process::std_out > is,
              boost::process::std_in < boost::process::null
              );


//    SSHCommand sc(remoteHostName_, {"bash -lc \""+escapeShellSymbols(cmd.str())+"\"" });
//    c.reset(new boost::process::child(
//              sc.command(),
//              boost::process::args(sc.arguments()),
//              boost::process::std_out > is
//              ));
  }
  else
  {
    c.reset(new boost::process::child(
              boost::process::search_path("tsp"),
              env_,
              boost::process::std_out > is
              ));
  }

  if (!c->running())
    throw insight::Exception("Could not execute task spooler executable!");

  std::vector<std::string> data;
  std::string line;
  int i=0;
  boost::regex re_q("^([^ ]*) +([^ ]*) +([^ ]*) +(.*)$");
  boost::regex re_f("^([^ ]*) +([^ ]*) +([^ ]*) +([^ ]*) +([^ ]*) +(.*)$");
  while (std::getline(is, line))
  {
    if (i>0)
    {
      boost::smatch m1;
      if (boost::regex_match(line, m1, re_q))
      {
        Job j;

        j.id=boost::lexical_cast<int>(m1[1]);

        if (m1[2]=="running")
          j.state=Running;
        else if (m1[2]=="queued")
          j.state=Queued;
        else if (m1[2]=="finished")
          j.state=Finished;
        else
          j.state=Unknown;

        if (j.state==Queued)
        {
          j.output="";
          j.commandLine=m1[4];
        }
        else if (j.state==Running)
        {
          j.output=string(m1[3]);
          j.commandLine=m1[4];
        }
        else if (j.state==Finished)
        {
          try {
            boost::smatch m2;
            boost::regex_match(line, m2, re_f);

            j.output=string(m2[3]);

            j.elevel=boost::lexical_cast<int>(m2[4]);

            j.commandLine=m2[6];
          }
          catch (...)
          {}
        }

        jl.push_back(j);
      }
    }
    i++;
  }

  c->wait();

  return jl;
}

int TaskSpoolerInterface::clean()
{
  if (server_)
  {
    std::ostringstream cmd;
    cmd << "TS_SOCKET="<<socket_.string()<<" tsp -C";

    return server_->executeCommand(
          cmd.str(),
          true
          );
//    SSHCommand sc(remoteHostName_, { "bash -lc \""+escapeShellSymbols(cmd.str())+"\"" });
//      return boost::process::system(
//            sc.command(), boost::process::args(sc.arguments())
//            );
  }
  else
  {
      return boost::process::system(
            boost::process::search_path("tsp"),
            boost::process::args("-C"),
            env_
            );
  }
}


int TaskSpoolerInterface::kill()
{
  if (server_)
  {
    std::ostringstream cmd;
    cmd << "TS_SOCKET="<<socket_.string()<<" tsp -k";

    return server_->executeCommand(
          cmd.str(), true );
//    SSHCommand sc(remoteHostName_, {"bash -lc \""+escapeShellSymbols(cmd.str())+"\""});
//      return boost::process::system(
//            sc.command(), boost::process::args(sc.arguments())
//            );
  }
  else
  {
      return boost::process::system(
            boost::process::search_path("tsp"),
            boost::process::args("-k"),
            env_
            );
  }
}



void TaskSpoolerInterface::read_start(void)
{
  // read until EOL, then pass to receivers
  async_read_until
      (
        *tail_cout_, *buf_cout_, "\n",
        std::bind
        (
          &TaskSpoolerInterface::read_complete,
          this,
          std::placeholders::_1, std::placeholders::_2
         )
       );
}

void TaskSpoolerInterface::read_complete(const boost::system::error_code& error, size_t /*bytes_transferred*/)
{
  if (!error)
  {
    std::string line;
    std::istream is(&(*buf_cout_));
    getline(is, line);

    // read completed, so process the data
    for (auto& receiver: receivers_)
      receiver(line);

    read_start(); // start waiting for another asynchronous read again
  }
}


void TaskSpoolerInterface::startTail(std::function<void(const std::string&)> receiver, bool blocking)
{
  receivers_.clear();

  stopTail();

  receivers_.push_back(receiver);

  ios_.reset(new boost::asio::io_service());
  buf_cout_.reset(new boost::asio::streambuf());
  tail_cout_.reset(new boost::process::async_pipe(*ios_));
  try
  {
    if (server_)
    {
      std::ostringstream cmd;
      cmd << "TS_SOCKET="<<socket_.string()<<" tsp -t";

//      SSHCommand sc(remoteHostName_, { "bash -lc \""+escapeShellSymbols(cmd.str())+"\"" });
      tail_c_ = server_->launchCommand(
                cmd.str(),
//                      sc.command(), boost::process::args(sc.arguments()),

                (boost::process::std_out & boost::process::std_err) > *tail_cout_,

                boost::process::on_exit(
                        [&](int, const std::error_code&) {
                          tail_cout_->close();
                        })
                      /*,
                                      ios_*/  // if ios_ is supplied along with on_exit, comm hangs!!
              );
    }
    else
    {
      tail_c_.reset(new boost::process::child(

                      boost::process::search_path("tsp"),
                      boost::process::args("-t"),

                      env_,

                      (boost::process::std_out & boost::process::std_err) > *tail_cout_,

                      boost::process::on_exit
                      (
                        [&](int, const std::error_code&) {
                          tail_cout_->close();
                        })
                      ));
    }
  }
  catch (const boost::process::process_error& e)
  {
    throw insight::Exception(std::string("Could not set up task spooler subprocess!\nMessage: ")+e.what());
  }

  read_start();

  ios_run_thread_.reset(new std::thread
  (
     [&]() {
      ios_->run();
     }
  ));

  if (!tail_c_->running())
    throw insight::Exception("Could not execute task spooler executable!");

  if (blocking)
    ios_run_thread_->join();
}


bool TaskSpoolerInterface::isTailRunning() const
{
  if (tail_c_)
  {
    if (tail_c_->valid())
    {
      if (tail_c_->running())
      {
        return true;
      }
    }
  }
  return false;
}

void TaskSpoolerInterface::stopTail()
{

  if (isTailRunning())
  {
    tail_c_->terminate();
  }

  if (ios_run_thread_)
  {
    if (ios_run_thread_->joinable())
    {
      ios_run_thread_->join();
    }
    ios_->stop();
    ios_->reset();
    ios_run_thread_.reset();
  }

  if (tail_c_)
  {
    tail_c_.reset();
  }

  if (tail_cout_)
   tail_cout_.reset();

  if (buf_cout_)
  {
    buf_cout_.reset();
  }

  if (ios_)
  {
    ios_.reset();
  }

}


int TaskSpoolerInterface::startJob(const std::vector<std::string>& commandline)
{
  if (server_)
  {
//    std::vector<std::string> args({remote_machine_, "TS_SOCKET=\""+socket_.string()+"\"", "tsp"});

    //std::copy( commandline.begin(), commandline.end(), std::back_inserter(args) );
    auto cmd = "TS_SOCKET=\""+socket_.string()+"\" tsp " + algorithm::join(commandline, " ");

//    SSHCommand sc(remoteHostName_, { "bash", "-lc", "\""+escapeShellSymbols(cmd)+"\"" } );
//    return boost::process::system(
//          sc.command(), boost::process::args(sc.arguments())
//          );
    return server_->executeCommand(
          cmd, true
          );
  }
  else
  {
    return boost::process::system(
          boost::process::search_path("tsp"),
          boost::process::args(commandline),
          env_
          );
  }
}

void TaskSpoolerInterface::cancelAllJobs()
{
  while ( kill() == 0 )
  {
    clean();
  }
}

int TaskSpoolerInterface::stopTaskspoolerServer()
{
  if (server_)
  {
    auto cmd = "TS_SOCKET=\""+socket_.string()+"\" tsp -K";
//    SSHCommand sc(remoteHostName_, {"bash", "-lc", "\""+escapeShellSymbols(cmd)+"\""});
//      return boost::process::system(
//            sc.command(), boost::process::args(sc.arguments())
//            );
    return server_->executeCommand(cmd, true);
  }
  else
  {
      return boost::process::system(
            boost::process::search_path("tsp"),
            boost::process::args("-K"),
            env_
            );
  }
}


}
