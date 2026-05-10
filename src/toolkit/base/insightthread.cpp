#include "insightthread.h"

namespace insight {

Thread::Thread()
{}


Thread::~Thread()
{
    if (thread_.joinable())
        thread_.join();
}

void Thread::interrupt()
{
    thread_.interrupt();
}


/**
   * @brief join
   * join thread and rethrow any exception, if there was no handler set
   * @return
   */
void Thread::join()
{
    thread_.join();
    if (exception_) std::rethrow_exception(exception_);
}

} // namespace insight
