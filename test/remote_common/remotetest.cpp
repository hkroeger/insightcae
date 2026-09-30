#include "remotetest.h"

#include <atomic>
#include <condition_variable>
#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <thread>

#include <boost/regex.hpp>
#include <boost/filesystem/fstream.hpp>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include "base/parameterset.h"
#include "base/parameters/subsetparameter.h"


namespace fs = boost::filesystem;


namespace insight {
namespace remotetest {




std::string format(const char* fmt, ...)
{
    char buf[8192];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return buf;
}




void check(bool condition, const std::string& message)
{
    if (!condition)
        throw CheckFailed(message);
}




void fail(const std::string& message)
{
    throw CheckFailed(message);
}




static std::atomic<int> abandonedThreadCount{0};

int abandonedThreads()
{
    return abandonedThreadCount;
}




void checkCompletesWithin(
    std::chrono::milliseconds timeout,
    std::function<void()> fn,
    const std::string& what )
{
    struct State
    {
        std::mutex mx;
        std::condition_variable cv;
        bool done=false;
        std::exception_ptr ex;
    };
    auto st = std::make_shared<State>();

    std::thread t(
        [st,fn]()
        {
            std::exception_ptr ex;
            try { fn(); }
            catch (...) { ex=std::current_exception(); }

            std::lock_guard<std::mutex> l(st->mx);
            st->ex=ex;
            st->done=true;
            st->cv.notify_all();
        });

    std::unique_lock<std::mutex> l(st->mx);
    if (!st->cv.wait_for(l, timeout, [st]{ return st->done; }))
    {
        l.unlock();
        t.detach();
        ++abandonedThreadCount;
        fail(format("%s: did not complete within %d ms (probably hanging)",
                    what.c_str(), int(timeout.count())));
    }
    l.unlock();
    t.join();

    if (st->ex)
        std::rethrow_exception(st->ex);
}




bool waitFor(
    std::function<bool()> condition,
    std::chrono::milliseconds timeout,
    std::chrono::milliseconds interval )
{
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (true)
    {
        if (condition()) return true;
        if (std::chrono::steady_clock::now() > deadline) return false;
        std::this_thread::sleep_for(interval);
    }
}




std::string IsolatedResult::description() const
{
    if (timedOut)
        return "child process did not finish in time (hang)";
    if (!exitedNormally)
        return format("child process crashed with signal %d (%s)", signal, strsignal(signal));
    if (exitCode!=0)
        return format("child process failed with exit code %d", exitCode);
    return "child process succeeded";
}




IsolatedResult runIsolated(
    std::function<void()> fn,
    std::chrono::milliseconds timeout )
{
    std::cout<<std::flush;
    std::cerr<<std::flush;

    pid_t pid = fork();
    if (pid<0)
        throw std::runtime_error("fork failed");

    if (pid==0)
    {
        // child
        int ret=0;
        try
        {
            fn();
        }
        catch (const std::exception& e)
        {
            std::cerr<<"  [isolated] exception: "<<e.what()<<std::endl;
            ret=1;
        }
        catch (...)
        {
            std::cerr<<"  [isolated] unknown exception"<<std::endl;
            ret=1;
        }
        std::cout<<std::flush;
        std::cerr<<std::flush;
        _exit(ret);
    }

    IsolatedResult r;
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (true)
    {
        int status=0;
        pid_t w = waitpid(pid, &status, WNOHANG);
        if (w==pid)
        {
            if (WIFEXITED(status))
            {
                r.exitedNormally=true;
                r.exitCode=WEXITSTATUS(status);
            }
            else if (WIFSIGNALED(status))
            {
                r.exitedNormally=false;
                r.signal=WTERMSIG(status);
            }
            return r;
        }
        if (std::chrono::steady_clock::now() > deadline)
        {
            ::kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            r.timedOut=true;
            return r;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}




void checkIsolated(
    std::function<void()> fn,
    const std::string& what,
    std::chrono::milliseconds timeout )
{
    auto r = runIsolated(fn, timeout);
    check(
        r.exitedNormally && !r.timedOut && r.exitCode==0,
        what+": "+r.description() );
}




TestRunner::TestRunner(const std::string& suite, int argc, char* argv[])
    : suite_(suite)
{
    std::cout<<"==== test suite "<<suite_;
    for (int i=1; i<argc; ++i) std::cout<<" "<<argv[i];
    std::cout<<" ===="<<std::endl;
}




void TestRunner::run(
    const std::string& name,
    std::function<void()> body,
    std::chrono::milliseconds watchdog )
{
    std::cout<<"---- case "<<name<<std::endl;

    // watchdog thread: terminate the process, if the case hangs
    struct WD
    {
        std::mutex mx;
        std::condition_variable cv;
        bool finished=false;
    };
    auto wd = std::make_shared<WD>();
    std::thread wdt(
        [wd,watchdog,name,this]()
        {
            std::unique_lock<std::mutex> l(wd->mx);
            if (!wd->cv.wait_for(l, watchdog, [wd]{ return wd->finished; }))
            {
                {
                    std::lock_guard<std::mutex> l2(mx_);
                    failures_.push_back(
                        name+": watchdog expired after "
                        +std::to_string(watchdog.count())+" ms (hanging)" );
                }
                std::cerr<<"FAIL (watchdog) "<<name<<std::endl;
                std::cout<<summary()<<std::flush;
                std::cerr<<std::flush;
                std::_Exit(1);
            }
        });

    std::string failure;
    bool skipped=false;
    try
    {
        body();
    }
    catch (const SkipCase& s)
    {
        skipped=true;
        std::cout<<"SKIP "<<name<<": "<<s.what()<<std::endl;
    }
    catch (const CheckFailed& f)
    {
        failure=f.what();
    }
    catch (const std::exception& e)
    {
        failure=std::string("unexpected exception: ")+e.what();
    }
    catch (...)
    {
        failure="unexpected unknown exception";
    }

    {
        std::lock_guard<std::mutex> l(wd->mx);
        wd->finished=true;
        wd->cv.notify_all();
    }
    wdt.join();

    std::lock_guard<std::mutex> l(mx_);
    if (skipped)
    {
        nSkipped_++;
    }
    else if (failure.empty())
    {
        nPassed_++;
        std::cout<<"PASS "<<name<<std::endl;
    }
    else
    {
        failures_.push_back(name+": "+failure);
        std::cout<<"FAIL "<<name<<": "<<failure<<std::endl;
    }
}




std::string TestRunner::summary() const
{
    std::ostringstream os;
    os<<"==== summary of "<<suite_<<": "
      <<nPassed_<<" passed, "
      <<failures_.size()<<" failed, "
      <<nSkipped_<<" skipped ===="<<std::endl;
    for (const auto& f: failures_)
        os<<"  FAILED: "<<f<<std::endl;
    return os.str();
}




int TestRunner::finish()
{
    std::cout<<summary()<<std::flush;
    if (failures_.size()>0)
        return 1;
    if (nPassed_==0 && nSkipped_>0)
        return SKIP_RETURN_CODE;
    return 0;
}




void TestRunner::exit()
{
    int ret = finish();
    std::cout<<std::flush;
    std::cerr<<std::flush;
    // don't run static destructors: abandoned threads might still use static objects
    std::_Exit(ret);
}




TemporaryDirectory::TemporaryDirectory(const std::string& prefix, bool keep)
    : keep_(keep)
{
    path_ = fs::temp_directory_path() / fs::unique_path(prefix+"-%%%%-%%%%-%%%%");
    fs::create_directories(path_);
}




TemporaryDirectory::~TemporaryDirectory()
{
    if (!keep_)
    {
        boost::system::error_code ec;
        // make sure, that read-only directories created by tests can be removed
        if (fs::exists(path_, ec))
        {
            for (fs::recursive_directory_iterator i(path_, ec), e; i!=e; i.increment(ec))
            {
                if (fs::is_directory(i->path(), ec))
                    fs::permissions(i->path(), fs::owner_all, ec);
            }
            fs::permissions(path_, fs::owner_all, ec);
        }
        fs::remove_all(path_, ec);
    }
}




std::string readFile(const boost::filesystem::path& file)
{
    fs::ifstream f(file, std::ios::binary);
    check(f.good(), "could not open file "+file.string());
    std::ostringstream os;
    os<<f.rdbuf();
    return os.str();
}




void writeFile(const boost::filesystem::path& file, const std::string& content)
{
    if (!file.parent_path().empty())
        fs::create_directories(file.parent_path());
    fs::ofstream f(file, std::ios::binary);
    f<<content;
    check(f.good(), "could not write file "+file.string());
}




static std::map<fs::path, std::string> directoryContents(const fs::path& d)
{
    std::map<fs::path, std::string> result;
    for (fs::recursive_directory_iterator i(d), e; i!=e; ++i)
    {
        auto rel = fs::relative(i->path(), d);
        if (fs::is_regular_file(i->path()))
            result[rel]=readFile(i->path());
        else if (fs::is_directory(i->path()))
            result[rel]="<dir>";
    }
    return result;
}




std::string compareDirectories(
    const boost::filesystem::path& a,
    const boost::filesystem::path& b )
{
    auto ca=directoryContents(a), cb=directoryContents(b);
    for (const auto& e: ca)
    {
        auto i=cb.find(e.first);
        if (i==cb.end())
            return e.first.string()+" is missing in "+b.string();
        if (i->second!=e.second)
            return e.first.string()+" differs";
    }
    for (const auto& e: cb)
    {
        if (ca.find(e.first)==ca.end())
            return e.first.string()+" is missing in "+a.string();
    }
    return std::string();
}




std::string uniqueName(const std::string& prefix)
{
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 0xffffff);
    return prefix+format("-%d-%06x", int(getpid()), dist(gen));
}




std::string env(const std::string& name, const std::string& defaultValue)
{
    if (const char* v = getenv(name.c_str()))
        return v;
    return defaultValue;
}




boost::filesystem::path analyzeExecutable()
{
    auto e = env("INSIGHT_TEST_ANALYZE_EXE");
    if (!e.empty())
        return e;
    return "analyze";
}




bool localProcessIsAlive(int pid)
{
    if (::kill(pid, 0)!=0)
        return false;

    // zombies count as dead
    std::ifstream stat("/proc/"+std::to_string(pid)+"/stat");
    std::string line;
    if (std::getline(stat, line))
    {
        auto rp = line.rfind(')');
        if (rp!=std::string::npos && rp+2<line.size())
            return line[rp+2]!='Z';
    }
    return true;
}




std::vector<int> localProcessesMatching(const std::string& pattern)
{
    std::vector<int> result;
    boost::regex re(pattern, boost::regex::extended);
    int self = getpid();

    for (fs::directory_iterator i("/proc"), e; i!=e; ++i)
    {
        auto name = i->path().filename().string();
        if (name.find_first_not_of("0123456789")!=std::string::npos)
            continue;

        int pid = std::stoi(name);
        if (pid==self) continue;

        std::ifstream cmdlinef( (i->path()/"cmdline").string(), std::ios::binary );
        std::string cmdline(
            (std::istreambuf_iterator<char>(cmdlinef)),
            std::istreambuf_iterator<char>() );
        for (auto& c: cmdline) if (c=='\0') c=' ';

        if (!cmdline.empty()
            && boost::regex_search(cmdline, re)
            && localProcessIsAlive(pid) )
        {
            result.push_back(pid);
        }
    }
    return result;
}




void writeDummyAnalysisInputFile(
    std::ostream& os,
    double executionTime,
    int deltaT_ms,
    bool emitError )
{
    insight::AnalysisParameterSet ps("Dummy Analysis");
    ps.setDouble("executionTime", executionTime);
    ps.setInt("deltaT", deltaT_ms);
    ps.setBool("emitError", emitError);
    ps.saveToStream(os, insight::hierarchicalData::Element::OutputProperties());
}




void writeDummyAnalysisInputFile(
    const boost::filesystem::path& file,
    double executionTime,
    int deltaT_ms,
    bool emitError )
{
    std::ostringstream os;
    writeDummyAnalysisInputFile(os, executionTime, deltaT_ms, emitError);
    writeFile(file, os.str());
}




} // namespace remotetest
} // namespace insight
