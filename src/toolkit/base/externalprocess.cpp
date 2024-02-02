#include "externalprocess.h"
#include "base/exception.h"

#include <boost/process/async.hpp>
#include <boost/asio/steady_timer.hpp>

namespace bp = boost::process;

namespace insight {



std::istream& extendedGetline(std::istream& is, std::string& t)
{
  t.clear();

  std::istream::sentry se(is, true);
  std::streambuf* sb = is.rdbuf();

  for(;;) {
    int c = sb->sbumpc();
    switch (c) {
    case '\n':
      return is;
    case '\r':
      if(sb->sgetc() == '\n')
                sb->sbumpc();
      return is;
    case std::streambuf::traits_type::eof():
      // Also handle the case when the last line has no line ending
      if(t.empty())
                is.setstate(std::ios::eofbit);
      return is;
    default:
      t += (char)c;
    }
  }
}

typedef boost::asio::buffers_iterator<
    boost::asio::streambuf::const_buffers_type> iterator;

std::pair<iterator, bool>
match_endline(iterator begin, iterator end)
{
  auto i = begin;
  while (i != end)
  {
    if (*i=='\n' || *i=='\r')
      return std::make_pair(i, true);
    i++;
  }
  return std::make_pair(i, false);
}


void Job::read_start_out()
{
  boost::asio::async_read_until(
      out_, buf_out_, match_endline,
   [&](const boost::system::error_code &error, std::size_t /*size*/)
   {
    if (!error)
    {
      // read a line
     std::string line;
     std::istream is(&(buf_out_));
     extendedGetline(is, line);

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
   err_, buf_err_, match_endline,
   [&](const boost::system::error_code &error, std::size_t /*size*/)
   {
    if (!error)
    {
      // read a line
     std::string line;
     std::istream is(&buf_err_);
     extendedGetline(is, line);

     // process this line
     if (processStdErr_) processStdErr_(line);

     // restart
     read_start_err();
    }
   }
  );
}


Job::Job()
  : out_(ios_), err_(ios_)
{}



Job::Job(
    const std::string &cmd,
    std::vector<std::string> argv
    )
  : out_(ios_), err_(ios_),
    process_(
          std::make_unique<boost::process::child>(
            bp::search_path(cmd),
            bp::args( argv ),
            bp::std_in < in_,
            bp::std_out > out_,
            bp::std_err > err_
          )
    )

{
  insight::CurrentExceptionContext ex("launch external command \""+cmd+"\"");
  if (!process_->running())
  {
        throw insight::Exception(
            "launching of external application as subprocess failed!\n"
            );
  }
}


Job::Job(
    const std::pair<boost::filesystem::path,std::vector<std::string> >& ca
)
    : Job(ca.first.string(), ca.second)
{
}

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
{
}

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

  read_start_out();
  read_start_err();

  boost::asio::steady_timer t(ios_);

  std::function<void(boost::system::error_code)> interruption_handler =
   [&] (boost::system::error_code) {
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

    t.expires_from_now(std::chrono::seconds( 1 ));
    if (process_->running())
      t.async_wait(interruption_handler);
   };
  interruption_handler({});
  ios_.run();
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
  return std::make_shared<Job>(cmd, argv);
}



} // namespace insight
