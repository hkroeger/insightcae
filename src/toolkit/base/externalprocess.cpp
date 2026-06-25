#include "externalprocess.h"
#include "base/exception.h"
#include "base/translations.h"
#include "base/insightthread.h"

#include <boost/process/async.hpp>
#include <boost/asio/steady_timer.hpp>
#include <iostream>

namespace bp = boost::process;

namespace insight {



template<class Stream = std::istream>
Stream& extendedGetline(Stream& is, std::string& t)
{
    t.clear();

    typename Stream::sentry se(is, true);
    auto *sb = is.rdbuf();

    while (is.good())
    {
        int c = sb->sbumpc();
        switch (c) {
        case '\n':
            return is;
        case '\r':
        {
            auto next=sb->sgetc();
            if (next == '\n')
            {
                sb->sbumpc();
            }
            return is;
        }
        case std::streambuf::traits_type::eof():
            // Also handle the case when the last line has no line ending
            if (t.empty())
                is.setstate(std::ios::eofbit);
            return is;
        default:
            t += (char)c;
        }
    }
    return is;
}



Job::Job()
{}



Job::Job(
    const std::string &cmd,
    std::vector<std::string> argv,
    bool searchCmdInPath
    )
{
  insight::CurrentExceptionContext ex(
        _("launch external command \"%s\" with arguments {%s}"),
        cmd.c_str(),
        ((argv.size()?"'":"")
         +boost::join(argv, "', '")+
         (argv.size()?"'":"")).c_str()
        );
  process_ =
        std::make_unique<boost::process::child>(
            searchCmdInPath ? bp::search_path(cmd) : cmd,
            bp::args( argv ),
            bp::std_in < in_,
            bp::std_out > out_,
            bp::std_err > err_
        );

  if (!process_->running())
  {
        throw insight::Exception(
            "launching of external application as subprocess failed!\n"
            );
  }
}




Job::Job(
    const std::pair<
        boost::filesystem::path,
        std::vector<std::string > >& ca,
    bool searchCmdInPath
)
    : Job(ca.first.string(), ca.second, searchCmdInPath)
{}




Job::~Job()
{
  if (isRunning())
  {
        process_->terminate();
        wait();
  }
}




void Job::forkProcess(
    std::unique_ptr<boost::process::child> child
)
{}




void Job::wait()
{
  if (process_)
  {
        try
        {
            process_->wait();
        }
        catch (boost::process::process_error& err)
        {
            // continue and avoid unhandled "waitpid failed: no child process" exception
            // in case of segfaulted child process
        }
  }
}




bool Job::isRunning() const
{
  return process_ && process_->running();
}




void Job::terminate()
{
  if (process_)
  {
        process_->terminate();
  }
}




std::ostream& Job::input()
{
  return in_;
}




void Job::closeInput()
{
  in_.pipe().close();
}




const boost::process::child &Job::process() const
{
  return *process_;
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

  wait(); // exit code is not set correctly, if this is skipped
}


void Job::ios_run_with_interruption(
    std::function<void(const std::string& line)> processStdOut,
    std::function<void(const std::string& line)> processStdErr )
{
  processStdOut_ = processStdOut;
  processStdErr_ = processStdErr;


  insight::Thread outReader, errReader;

  outReader.launch([this](){
      std::string line;
      while (extendedGetline<std::istream>(out_, line))
      {
          if (processStdOut_)
          {
              processStdOut_(line);
          }
      }
  });

  errReader.launch([this](){
      std::string line;
      while (extendedGetline<std::istream>(err_, line))
      {
          if (processStdErr_)
          {
              processStdErr_(line);
          }
      }
  });


  while (process_->running())
  {
      try
      {
          boost::this_thread::interruption_point();
      }
      catch (const boost::thread_interrupted& i)
      {
#ifdef WIN32
          process_->terminate();
#else
          ::kill(process_->id(), SIGTERM);
#endif
          throw i;
      }
  }
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
                    if (oa->stopIsDemanded())
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
  return std::make_shared<Job>(cmd, argv);
}



} // namespace insight
