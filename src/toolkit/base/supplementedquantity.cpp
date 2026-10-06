#include "supplementedquantity.h"

#include "base/exceptionhandling.h"
#include "base/progressdisplayer.h"
#include "base/warningdispatcher.h"

namespace insight {


namespace {

/**
 * receives the progress of deferred computations,
 * for which no progress output was requested
 */
class SilentProgressDisplayer
    : public ProgressDisplayer
{
public:
    void setActionProgressValue(const std::string&, double) override {}
    void setMessageText(const std::string&, const std::string&) override {}
    void finishActionProgress(const std::string&) override {}
    void reset() override {}
    void update(const ProgressState&) override {}
    void logMessage(const std::string&) override {}
};

SilentProgressDisplayer silentProgressDisplayer;

}



std::string describeException(std::exception_ptr ex)
{
    if (!ex) return std::string();
    try
    {
        std::rethrow_exception(ex);
    }
    catch (const SupplementedQuantityError& e)
    {
        return e.rootMessage();
    }
    catch (const ExceptionBase& e)
    {
        return e.message();
    }
    catch (const std::exception& e)
    {
        return e.what();
    }
    catch (...)
    {
        auto d=describeCurrentException();
        return d ? std::string(*d) : std::string("unknown error");
    }
}




namespace {

struct RootCause
{
    std::string quantity, message;
    std::exception_ptr cause;

    RootCause(const std::string& accessedQuantity, std::exception_ptr ex)
        : quantity(accessedQuantity), cause(ex)
    {
        try
        {
            std::rethrow_exception(ex);
        }
        catch (const SupplementedQuantityError& e)
        {
            // error in dependency: keep the original reason
            quantity=e.rootQuantity();
            message=e.rootMessage();
            cause=e.rootCause();
        }
        catch (...)
        {
            message=describeException(ex);
        }
    }

    std::string errorMessage(const std::string& accessedQuantity) const
    {
        if (quantity!=accessedQuantity)
            return "supplemented quantity \""+accessedQuantity+"\" is unavailable,"
                   " since \""+quantity+"\" could not be computed: "+message;
        else
            return "supplemented quantity \""+accessedQuantity+"\" could not be computed: "+message;
    }
};

}




SupplementedQuantityError::SupplementedQuantityError(
    const std::string& quantity,
    std::exception_ptr cause )
    : Exception("%s", RootCause(quantity, cause).errorMessage(quantity).c_str()),
    quantity_(quantity)
{
    RootCause rc(quantity, cause);
    rootQuantity_=rc.quantity;
    rootMessage_=rc.message;
    rootCause_=rc.cause;
}


const std::string& SupplementedQuantityError::quantity() const
{
    return quantity_;
}

const std::string& SupplementedQuantityError::rootQuantity() const
{
    return rootQuantity_;
}

const std::string& SupplementedQuantityError::rootMessage() const
{
    return rootMessage_;
}

std::exception_ptr SupplementedQuantityError::rootCause() const
{
    return rootCause_;
}

bool SupplementedQuantityError::isDependencyFailure() const
{
    return rootQuantity_!=quantity_;
}




SupplementedQuantityBase::SupplementedQuantityBase(
    const std::string& name,
    ProgressSource progressSource,
    IssueSink issueSink )
    : name_(name),
    progressSource_(progressSource),
    issueSink_(issueSink),
    status_(Pending)
{}


SupplementedQuantityBase::~SupplementedQuantityBase()
{}


const std::string& SupplementedQuantityBase::name() const
{
    return name_;
}


SupplementedQuantityBase::Status SupplementedQuantityBase::status() const
{
    boost::mutex::scoped_lock lck(mtx_);
    return status_;
}


std::exception_ptr SupplementedQuantityBase::error() const
{
    boost::mutex::scoped_lock lck(mtx_);
    return error_;
}


bool SupplementedQuantityBase::claim()
{
    boost::mutex::scoped_lock lck(mtx_);
    if (status_!=Pending) return false;
    status_=Running;
    runningIn_=boost::this_thread::get_id();
    return true;
}


void SupplementedQuantityBase::execute()
{
    std::exception_ptr err;
    bool interrupted=false;
    try
    {
        CurrentExceptionContext ec("computing supplemented quantity \""+name_+"\"");

        // warnings issued during this computation belong to this quantity
        ScopedWarningRecorder warnings(
            [this](const insight::Exception& w)
            {
                if (issueSink_) issueSink_(name_, w.message());
            } );

        ActionProgressPtr ap;
        if (progressSource_) ap=progressSource_(name_);
        if (!ap) ap=silentProgressDisplayer.forkNewAction(1, name_);
        compute(*ap);
    }
    catch (const boost::thread_interrupted&)
    {
        interrupted=true;
        err=std::make_exception_ptr(
            Exception("computation was cancelled") );
    }
    catch (...)
    {
        err=std::current_exception();
    }

    {
        boost::mutex::scoped_lock lck(mtx_);
        error_=err;
        status_= err ? Failed : Done;
        runningIn_=boost::thread::id();
    }
    cv_.notify_all();

    if (interrupted)
        throw boost::thread_interrupted();
}


bool SupplementedQuantityBase::tryRun()
{
    if (!claim()) return false;
    execute();
    return true;
}


void SupplementedQuantityBase::wait() const
{
    // not started yet: compute in this thread
    if (const_cast<SupplementedQuantityBase*>(this)->tryRun())
        return;

    boost::mutex::scoped_lock lck(mtx_);
    if (status_==Running && runningIn_==boost::this_thread::get_id())
    {
        throw insight::Exception(
            "cyclic dependency detected: supplemented quantity \"%s\" depends on itself",
            name_.c_str() );
    }
    while (status_==Running)
    {
        cv_.wait(lck); // interruption point
    }
}


void SupplementedQuantityBase::ensureComputed() const
{
    wait();
    boost::mutex::scoped_lock lck(mtx_);
    if (status_==Failed)
        throw SupplementedQuantityError(name_, error_);
}




} // namespace insight
