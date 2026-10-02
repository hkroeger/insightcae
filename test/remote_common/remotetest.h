#ifndef INSIGHT_REMOTETEST_H
#define INSIGHT_REMOTETEST_H

/*
 * Minimal test harness for the remote execution tests.
 *
 * Follows the convention of the other InsightCAE tests (plain main(),
 * non-zero exit code on failure) but allows to run several named cases
 * within one executable, so that one failing case does not hide the
 * results of the others.
 *
 * Hangs are turned into failures:
 *  - checkCompletesWithin() runs a function in a separate thread
 *    and reports a failure, if it does not return in time.
 *  - each case has a watchdog. If it fires, the summary is printed
 *    and the process is terminated with a failure exit code.
 *
 * Crashes are turned into failures by runIsolated(), which executes
 * a function in a forked child process.
 */

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <thread>

#include <boost/filesystem.hpp>


namespace insight {
namespace remotetest {


/**
 * exit code, which is interpreted by CTest as "skipped"
 * (requires SKIP_RETURN_CODE property)
 */
constexpr int SKIP_RETURN_CODE = 77;


struct CheckFailed : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

/**
 * @brief The SkipCase struct
 * throw to skip the current case
 */
struct SkipCase : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};


std::string format(const char* fmt, ...);


void check(bool condition, const std::string& message);

template<class ...Args>
void check(bool condition, const char* fmt, Args&&... args)
{
    if (!condition)
        check(false, format(fmt, std::forward<Args>(args)...));
}

void fail(const std::string& message);


/**
 * @brief expectThrows
 * check that fn throws an exception of type E
 * @return the message of the caught exception
 */
template<class E = std::exception>
std::string expectThrows(std::function<void()> fn, const std::string& what)
{
    try
    {
        fn();
    }
    catch (const E& e)
    {
        return e.what();
    }
    catch (const std::exception& e)
    {
        fail(what+": expected a different exception type, got: "+e.what());
    }
    catch (...)
    {
        fail(what+": expected an exception derived from std::exception, got something else");
    }
    fail(what+": expected an exception, but none was thrown");
    return std::string();
}


/**
 * @brief checkCompletesWithin
 * run fn in a separate thread and fail, if it does not complete within timeout.
 * Exceptions thrown by fn are rethrown.
 * In case of a timeout, the thread is abandoned (it may hang forever).
 * fn must therefore own (e.g. through captured shared_ptrs) everything it uses.
 */
void checkCompletesWithin(
    std::chrono::milliseconds timeout,
    std::function<void()> fn,
    const std::string& what );


/**
 * @brief waitFor
 * poll condition until it becomes true or the timeout expires
 * @return true, if condition became true
 */
bool waitFor(
    std::function<bool()> condition,
    std::chrono::milliseconds timeout,
    std::chrono::milliseconds interval = std::chrono::milliseconds(50) );


struct IsolatedResult
{
    bool exitedNormally = false;
    int exitCode = -1;
    int signal = 0;
    bool timedOut = false;
    std::string description() const;
};

/**
 * @brief runIsolated
 * execute fn in a forked child process.
 * (On Windows, where fork() is not available, fn is executed in a separate thread:
 * exceptions and hangs are detected, but crashes are not isolated.)
 * The child exits with 0, if fn returns normally,
 * and with 1, if it throws (the message is printed to stderr).
 * Crashes (signals) and hangs (timeout) are reported in the result.
 */
IsolatedResult runIsolated(
    std::function<void()> fn,
    std::chrono::milliseconds timeout = std::chrono::seconds(60) );

/**
 * @brief checkIsolated
 * run fn in a child process and fail, if it crashes, hangs or throws
 */
void checkIsolated(
    std::function<void()> fn,
    const std::string& what,
    std::chrono::milliseconds timeout = std::chrono::seconds(60) );



class TestRunner
{
    std::string suite_;
    int nPassed_=0, nSkipped_=0;
    std::vector<std::string> failures_;
    std::mutex mx_;

public:
    TestRunner(const std::string& suite, int argc=0, char* argv[]=nullptr);

    /**
     * @brief run
     * execute a named test case
     * @param watchdog
     * maximum execution time of the case.
     * When exceeded, the process is terminated with a failure.
     */
    void run(
        const std::string& name,
        std::function<void()> body,
        std::chrono::milliseconds watchdog = std::chrono::minutes(3) );

    std::string summary() const;

    /**
     * @brief finish
     * print summary
     * @return exit code: 0 success, 1 failure, SKIP_RETURN_CODE if all cases were skipped
     */
    int finish();

    /**
     * @brief exit
     * print summary and terminate the process immediately
     * (without running static destructors, which might block
     * because of abandoned threads)
     */
    [[noreturn]] void exit();
};


/**
 * @brief abandonedThreads
 * number of threads, which have been abandoned by checkCompletesWithin
 */
int abandonedThreads();


// ======== file system helpers

class TemporaryDirectory
{
    boost::filesystem::path path_;
    bool keep_;
public:
    TemporaryDirectory(const std::string& prefix = "insight-remotetest", bool keep=false);
    ~TemporaryDirectory();
    const boost::filesystem::path& path() const { return path_; }
    operator const boost::filesystem::path&() const { return path_; }
};

std::string readFile(const boost::filesystem::path& file);
void writeFile(const boost::filesystem::path& file, const std::string& content);

/**
 * @brief compareDirectories
 * compares file names and file contents recursively
 * @return empty string, if identical, otherwise a description of the first difference
 */
std::string compareDirectories(
    const boost::filesystem::path& a,
    const boost::filesystem::path& b );

std::string uniqueName(const std::string& prefix);


// ======== environment

std::string env(const std::string& name, const std::string& defaultValue = std::string());

void setEnv(const std::string& name, const std::string& value);

/**
 * @brief pathListSeparator
 * separator of the entries in PATH (':' or ';' on Windows)
 */
char pathListSeparator();

/**
 * @brief analyzeExecutable
 * path to the analyze executable.
 * Taken from the environment variable INSIGHT_TEST_ANALYZE_EXE (set by CMake),
 * otherwise "analyze" (searched in PATH).
 */
boost::filesystem::path analyzeExecutable();


// ======== process helpers

int currentProcessId();

bool localProcessIsAlive(int pid);

/**
 * @brief localProcessesMatching
 * @param pattern
 * extended regex, matched against the full command line (like "pgrep -f")
 * @return PIDs of matching processes (excluding the calling process)
 */
std::vector<int> localProcessesMatching(const std::string& pattern);


/**
 * @brief writeDummyAnalysisInputFile
 * write an input file for the "Dummy Analysis" (insight-dummies addon)
 */
void writeDummyAnalysisInputFile(
    std::ostream& os,
    double executionTime,
    int deltaT_ms,
    bool emitError );

void writeDummyAnalysisInputFile(
    const boost::filesystem::path& file,
    double executionTime,
    int deltaT_ms,
    bool emitError );


} // namespace remotetest
} // namespace insight

#endif // INSIGHT_REMOTETEST_H
