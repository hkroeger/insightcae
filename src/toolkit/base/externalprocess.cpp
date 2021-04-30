#include "externalprocess.h"
#include "base/exception.h"

#include <boost/asio.hpp>
#include <boost/process/async.hpp>
#include <boost/asio/steady_timer.hpp>

namespace bp = boost::process;

namespace insight {



void Job::read_start_out()
{
  boost::asio::async_read_until(
   out, buf_out, "\n",
   [&](const boost::system::error_code &error, std::size_t /*size*/)
   {
    if (!error)
    {
      // read a line
     std::string line;
     std::istream is(&(buf_out));
     getline(is, line);

     // process line
     if (processStdOut_) processStdOut_(line);

     // restart read
     read_start_out();
    }
   }
  );
}


void Job::read_start_err()
{
  boost::asio::async_read_until(
   err, buf_err, "\n",
   [&](const boost::system::error_code &error, std::size_t /*size*/)
   {
    if (!error)
    {
      // read a line
     std::string line;
     std::istream is(&buf_err);
     getline(is, line);

     // process this line
     if (processStdErr_) processStdErr_(line);

     // restart
     read_start_err();
    }
   }
  );
}


Job::Job()
  : out(ios), err(ios)
{
}




void Job::runAndTransferOutput
(
    std::vector<std::string>* stdout,
    std::vector<std::string>* stderr
)
{

  ios_run_with_interruption(
        [stdout](const std::string& line)
        {
          std::cout<<line<<std::endl;
          if (stdout) stdout->push_back(line);
        },
        [stdout,stderr](const std::string& line)
        {
          // mirror to console
          std::cout<<"[E] "<<line<<std::endl;
          if (stderr)
          {
            stderr->push_back(line);
          }
          else
          {
            if (stdout)
              stdout->push_back("[E] "+line);
          }
        }
  );

  process->wait(); // exit code is not set correctly, if this is skipped
}


void Job::ios_run_with_interruption(
    std::function<void(const std::string& line)> processStdOut,
    std::function<void(const std::string& line)> processStdErr )
{
  processStdOut_ = processStdOut;
  processStdErr_ = processStdErr;

  read_start_out();
  read_start_err();

  boost::asio::steady_timer t(ios);

  std::function<void(boost::system::error_code)> interruption_handler =
   [&] (boost::system::error_code) {
    try
    {
      boost::this_thread::interruption_point();
    }
    catch (const boost::thread_interrupted& i)
    {
      process->terminate();
      throw i;
    }

    t.expires_from_now(std::chrono::seconds( 1 ));
    if (process->running())
      t.async_wait(interruption_handler);
   };
  interruption_handler({});
  ios.run();
}




JobPtr forkExternalProcess
(
    const std::string& cmd,
    std::vector<std::string> argv
)
{
  auto job = std::make_shared<Job>();

  job->process.reset(
        new bp::child(
           bp::search_path(cmd),
           bp::args( argv ),
           bp::std_in < job->in,
           bp::std_out > job->out,
           bp::std_err > job->err
          )
        );

  if (!job->process->running())
  {
    //throw insight::Exception("SoftwareEnvironment::forkCommand(): Failed to launch subprocess!\n(Command was \""+dbgs.str()+"\")");
    throw insight::Exception(
              boost::str(boost::format(
                 "Launching of external application \"%s\" as subprocess failed!\n")
                  % cmd)
              );
  }

  std::cout<<"Executing "<<cmd<<std::endl;

  return job;
}


} // namespace insight
