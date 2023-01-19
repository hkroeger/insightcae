#include "externalprocess.h"
#include "base/exception.h"

#include <boost/process/async.hpp>
#include <boost/asio/steady_timer.hpp>

namespace bp = boost::process;

namespace insight {



ExternalProcess::ExternalProcess()
{}


ExternalProcess::ExternalProcess(std::unique_ptr<boost::process::child> process)
    : process_(std::move(process))
{}

ExternalProcess::~ExternalProcess()
{
    if (isRunning())
    {
        process_->terminate();
        wait();
    }
}



void ExternalProcess::wait()
{
    if (process_)
    {
        process_->wait();
    }
}



bool ExternalProcess::isRunning() const
{
    return process_ && process_->running();
}

void ExternalProcess::terminate()
{
    if (process_)
    {
        process_->terminate();
    }
}

const boost::process::child &ExternalProcess::process() const
{
    return *process_;
}





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
    std::vector<std::string>* stdoutbuffer,
    std::vector<std::string>* stderrbuffer,
    bool mirrorStdout,
    bool mirrorStderr
)
{

  ios_run_with_interruption(
        [stdoutbuffer,mirrorStdout](const std::string& line)
        {
          if (mirrorStdout) std::cout<<line<<std::endl;
          if (stdoutbuffer) stdoutbuffer->push_back(line);
        },
        [stdoutbuffer,stderrbuffer,mirrorStderr](const std::string& line)
        {
          // mirror to console
          if (mirrorStderr) std::cout<<"[E] "<<line<<std::endl;
          if (stderrbuffer)
          {
              stderrbuffer->push_back(line);
          }
          else if (stdoutbuffer)
          {
              stdoutbuffer->push_back("[E] "+line);
          }
        }
  );

  process_->wait(); // exit code is not set correctly, if this is skipped
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
      process_->terminate();
      throw i;
    }

    t.expires_from_now(std::chrono::seconds( 1 ));
    if (process_->running())
      t.async_wait(interruption_handler);
   };
  interruption_handler({});
  ios.run();
}



void Job::ios_run_with_interruption(OutputAnalyzer *oa)
{
    ios_run_with_interruption(

              [&](const std::string& line)
              {
                std::cout<<line<<std::endl; // mirror to console

                if (oa)
                {
                    oa->update(line);
                    if (oa->stopRun())
                    {
                      terminate();
                    }
                }
              },

              [&](const std::string& line)
              {
                // mirror to console
                std::cout<<"[E] "<<line<<std::endl; // mirror to console
                if (oa)
                {
                    oa->update("[E] "+line);
                }
              }
        );
}




void Job::forkExternalProcess
(
    JobPtr job,
    std::unique_ptr<boost::process::child> child
)
{
  job->process_ = std::move(child);
  if (!job->process_->running())
  {
   throw insight::Exception(
                "launching of external application as subprocess failed!\n"
             );
  }
}




JobPtr Job::forkExternalProcess
(
    const std::string& cmd,
    std::vector<std::string> argv
)
{
  insight::CurrentExceptionContext ex("launch external command \""+cmd+"\"");
  auto job = std::make_shared<Job>();
  forkExternalProcess(
        job,
        std::make_unique<boost::process::child>(
          bp::search_path(cmd),
          bp::args( argv ),
          bp::std_in < job->in,
          bp::std_out > job->out,
          bp::std_err > job->err
          )
        );
  return job;
}



} // namespace insight
