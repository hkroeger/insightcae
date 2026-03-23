

#include "boost/asio.hpp"
#include "boost/process.hpp"
#include <boost/process/async.hpp>
#include <boost/process/io.hpp>
#include <boost/asio/steady_timer.hpp>
#include "boost/thread.hpp"
#include <iostream>
#include <codecvt>

#include "base/insightthread.h"

namespace bp = boost::process;

namespace insight_test {

template<class Stream>
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





class Job
{

protected:
    boost::process::opstream in_;
    boost::process::ipstream out_, err_;

    std::unique_ptr<boost::process::child> process_;


public:
    Job()
    {
        process_ =
            std::make_unique<boost::process::child>(
#ifdef WIN32
                "c:\\Windows\\Sysnative\\wsl.exe"
#else
                "/bin/bash"
#endif
                ,
                bp::args( {
#ifdef WIN32
                      "-d", "Ubuntu", "--", "/bin/bash", "-lc",
#else
                      "-lc",
#endif
                      "rsync -av --dry-run --info=progress2 /usr/share/doc/man-db /tmp"
                } ),
                bp::std_in < in_,
                bp::std_out > out_,
                bp::std_err > err_
                // ios_
                );

        if (!process_->running())
        {
            throw std::runtime_error(
                "launching of external application as subprocess failed!\n"
                );
        }
    }

    void ios_run_with_interruption( )
    {
        insight::Thread outReader([this](){
            std::string line;
            while (extendedGetline<std::istream>(out_, line))
            {
                std::cout<<"O >"<<line<<"<\r\n";
            }
            std::cout<<"end out"<<std::endl;
        });

        insight::Thread errReader([this](){
            std::string line;
            while (extendedGetline<std::istream>(err_, line))
            {
                std::cout<<"E >"<<line<<"<\r\n";
            }
            std::cout<<"end err"<<std::endl;
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


};



}


int main(int argc, char *argv[])
{
    insight_test::Job j;
    j.ios_run_with_interruption();
    return 0;
}
