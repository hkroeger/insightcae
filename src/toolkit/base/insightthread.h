#ifndef INSIGHTTHREAD_H
#define INSIGHTTHREAD_H

#include <functional>
#include "base/progressdisplayer.h"
#include "base/exception.h"

#include "base/boost_include.h"




namespace insight {



template<
    typename InterruptHandlerType = std::function<void(void)>,
    typename ExceptionHandlerType = std::function<void(std::exception_ptr)> >
class Thread
{
public:
    typedef InterruptHandlerType InterruptHandler;
    typedef ExceptionHandlerType ExceptionHandler;

private:
    boost::thread thread_;

    ExceptionHandler exceptionHandler_;
    InterruptHandler interruptHandler_;

    /**
   * @brief exception
   * stores the exception, if any occurred
   */
    std::exception_ptr exception_;

public:
    Thread(
        ExceptionHandler exHdlr = ExceptionHandler(),
        InterruptHandler intHdlr = InterruptHandler()
        )
      : exceptionHandler_(exHdlr),
        interruptHandler_(intHdlr)
    {}

    template<class Function>
    Thread(
        Function functionToExecute,
        ExceptionHandler exHdlr = ExceptionHandler(),
        InterruptHandler intHdlr = InterruptHandler()
        )
        : Thread(exHdlr, intHdlr)
    {
        launch(functionToExecute);
    }

    ~Thread()
    {
        if (thread_.joinable())
            thread_.join();
    }


    template<class Function>
    void launch(
        Function functionToExecute )
    {
        thread_ = boost::thread(

            [this,functionToExecute](WarningDispatcher* globalWarning)
            {
                try
                {
                    WarningDispatcher::getCurrent().setSuperDispatcher(globalWarning);

                    functionToExecute();
                }
                catch (const boost::thread_interrupted& i)
                {
                    if (interruptHandler_)
                    {
                        interruptHandler_();
                    }
                }
                catch (...)
                {
                    auto e = std::current_exception();
                    if (exceptionHandler_)
                    {
                        exceptionHandler_(e);
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






    void interrupt()
    {
        thread_.interrupt();
    }


  /**
   * @brief join
   * join thread and rethrow any exception, if there was no handler set
   * @return
   */
    void join()
    {
        thread_.join();
        if (exception_) std::rethrow_exception(exception_);
    }




    template <class Rep, class Period>
    bool try_join_for(const boost::chrono::duration<Rep, Period>& rel_time)
    {
        return thread_.try_join_for(rel_time);
    }
};




} // namespace insight




#endif // THREAD_H
