#ifndef INSIGHTTHREAD_H
#define INSIGHTTHREAD_H

#include <functional>
#include "base/progressdisplayer.h"
#include "base/exception.h"

#include <boost/thread.hpp>



namespace insight {




class Thread
{
private:
    boost::thread thread_;

    /**
   * @brief exception
   * stores the exception, if any occurred
   */
    std::exception_ptr exception_;

public:
    Thread();
    virtual ~Thread();

    template<
        class Function,
        typename ExceptionHandler = std::function<void(std::exception_ptr)>,
        typename InterruptHandler = std::function<void(void)>
        >
    void launch(
        Function functionToExecute,
        ExceptionHandler exHndlr = ExceptionHandler(),
        InterruptHandler intHndlr = InterruptHandler() )
    {
        insight::assertion(
            !thread_.joinable(),
            "launch() called on already-running task" );

        thread_ = boost::thread(

            [this,functionToExecute,exHndlr,intHndlr](
                WarningDispatcher* globalWarning)
            {
                try
                {
                    WarningDispatcher::getCurrent().setSuperDispatcher(globalWarning);

                    functionToExecute();
                }
                catch (const boost::thread_interrupted& i)
                {
                    if (intHndlr)
                    {
                        intHndlr();
                    }
                }
                catch (...)
                {
                    auto e = std::current_exception();
                    if (exHndlr)
                    {
                        exHndlr(e);
                    }
                    else
                    {
                        exception_ = e;
                    }
                }
            },

            &WarningDispatcher::getCurrent()
            );
    }


    void interrupt();

    /**
   * @brief join
   * join thread and rethrow any exception, if there was no handler set
   * @return
   */
    void join();


    template <class Rep, class Period>
    bool try_join_for(const boost::chrono::duration<Rep, Period>& rel_time)
    {
        return thread_.try_join_for(rel_time);
    }
};






} // namespace insight




#endif // THREAD_H
