#include "supplementedinputdata.h"
#include "base/cppextensions.h"
#include "base/exception.h"
#include "base/parameterset.h"
#include "base/warningdispatcher.h"
#include <string>

namespace insight {







supplementedInputDataBase::supplementedInputDataBase(
    const boost::filesystem::path &exePath )
    : executionPath_(exePath),
    progressMutex_(std::make_shared<boost::mutex>())
{}

supplementedInputDataBase::supplementedInputDataBase(
    ParameterSetInput&& ip,
    const boost::filesystem::path& exePath,
    ActionProgress& )
  : executionPath_(exePath),
    progressMutex_(std::make_shared<boost::mutex>())
{
#warning check, if can be avoided
    if (ip.hasParameterSet())
    {
        parameters_ = &ip.parameterSet();
    }
}

supplementedInputDataBase::~supplementedInputDataBase()
{
    bool running=false;
    {
        boost::mutex::scoped_lock lck(deferredMutex_);
        running=!deferredWorkers_.empty();
    }
    if (running)
    {
        // derived class is already destroyed at this point but the workers
        // might still access its members. Owners have to call
        // computeAll() or cancelDeferredComputations() before.
        std::cerr
            << "Internal error: supplemented input data destroyed"
               " while deferred computations are running!"
            << std::endl;
        cancelDeferredComputations();
    }
}




void supplementedInputDataBase::registerDeferred(SupplementedQuantityBasePtr q)
{
    boost::mutex::scoped_lock lck(deferredMutex_);
    deferred_.push_back(q);
}




SupplementedQuantityBase::ProgressSource
supplementedInputDataBase::progressSource()
{
    auto mtx=progressMutex_;
    return [this,mtx](const std::string& name) -> ActionProgressPtr
    {
        boost::mutex::scoped_lock lck(*mtx);
        if (!deferredProgress_)
            return nullptr;

        // ActionProgress is not thread safe: fork and release
        // of child actions need to be serialized
        auto child=deferredProgress_->forkNewAction(1, name);
        return ActionProgressPtr(
            child.get(),
            [child,mtx](ActionProgress*) mutable
            {
                boost::mutex::scoped_lock lck(*mtx);
                child.reset();
            } );
    };
}




void supplementedInputDataBase::launchAll(ActionProgressPtr progress)
{
    boost::mutex::scoped_lock lck(deferredMutex_);

    if (!deferredWorkers_.empty())
        return; // already launched

    std::vector<SupplementedQuantityBasePtr> pending;
    std::copy_if(
        deferred_.begin(), deferred_.end(),
        std::back_inserter(pending),
        [](const SupplementedQuantityBasePtr& q)
        { return q->status()==SupplementedQuantityBase::Pending; } );

    if (pending.empty())
        return;

    {
        boost::mutex::scoped_lock plck(*progressMutex_);
        if (progress)
            deferredProgress_=progress;
    }

    auto* mainThreadWD = &WarningDispatcher::getCurrent();
    int nThreads = std::min<int>(
        std::max<int>(1, boost::thread::hardware_concurrency()),
        pending.size() );

    for (int i=0; i<nThreads; ++i)
    {
        deferredWorkers_.push_back(
            std::make_unique<boost::thread>(
                [pending,mainThreadWD]()
                {
                    WarningDispatcher::getCurrent().setSuperDispatcher(mainThreadWD);
                    try
                    {
                        // each worker visits all quantities:
                        // whatever is not yet claimed by another thread is computed here
                        for (auto& q: pending)
                            q->tryRun();
                    }
                    catch (...)
                    {
                        // interrupted (or unexpected error, which is stored in the quantity anyway)
                    }
                } ) );
    }
}




void supplementedInputDataBase::computeAll(ActionProgressPtr progress)
{
    launchAll(progress);

    // the workers remain owned by this object until they are joined:
    // if the calling thread is interrupted while waiting,
    // cancelDeferredComputations() still finds them
    std::vector<boost::thread*> workers;
    {
        boost::mutex::scoped_lock lck(deferredMutex_);
        for (auto& w: deferredWorkers_)
            workers.push_back(w.get());
    }
    for (auto* w: workers)
    {
        if (w->joinable())
            w->join(); // interruption point
    }
    {
        boost::mutex::scoped_lock lck(deferredMutex_);
        deferredWorkers_.clear();
    }

    // compute also quantities, which were added after launch
    decltype(deferred_) all;
    {
        boost::mutex::scoped_lock lck(deferredMutex_);
        all=deferred_;
    }
    for (auto& q: all)
        q->wait();

    releaseDeferredProgress();
}




void supplementedInputDataBase::cancelDeferredComputations()
{
    // has to complete, even if the calling thread is interrupted
    boost::this_thread::disable_interruption di;

    decltype(deferredWorkers_) workers;
    {
        boost::mutex::scoped_lock lck(deferredMutex_);
        workers.swap(deferredWorkers_);
    }
    for (auto& w: workers)
        w->interrupt();
    for (auto& w: workers)
    {
        if (w->joinable())
            w->join();
    }

    releaseDeferredProgress();
}




void supplementedInputDataBase::releaseDeferredProgress()
{
    // the progress display of the parent action ends.
    // Later on-demand computations are not displayed.
    boost::mutex::scoped_lock plck(*progressMutex_);
    deferredProgress_.reset();
}




bool supplementedInputDataBase::complete() const
{
    boost::mutex::scoped_lock lck(deferredMutex_);
    return std::all_of(
        deferred_.begin(), deferred_.end(),
        [](const SupplementedQuantityBasePtr& q)
        { return q->status()==SupplementedQuantityBase::Done; } );
}




supplementedInputDataBase::ErrorList
supplementedInputDataBase::errors() const
{
    ErrorList errs;
    boost::mutex::scoped_lock lck(deferredMutex_);
    for (auto& q: deferred_)
    {
        if (q->status()==SupplementedQuantityBase::Failed)
            errs.push_back({q->name(), q->error()});
    }
    return errs;
}




supplementedInputDataBase::ErrorList
supplementedInputDataBase::rootErrors() const
{
    ErrorList errs;
    for (auto& e: errors())
    {
        try
        {
            std::rethrow_exception(e.second);
        }
        catch (const SupplementedQuantityError&)
        {
            // caused by a dependency
        }
        catch (...)
        {
            errs.push_back(e);
        }
    }
    return errs;
}




void supplementedInputDataBase::throwIfIncomplete(ActionProgressPtr progress)
{
    computeAll(progress);

    auto errs=errors();
    if (!errs.empty())
    {
        auto rerrs=rootErrors();
        if (rerrs.empty()) rerrs=errs; // root cause outside of this object

        std::ostringstream os;
        os << "The input data could not be processed."
              " The following errors occurred:\n";
        for (auto& e: rerrs)
            os << " * " << e.first << ": " << describeException(e.second) << "\n";

        if (errs.size()>rerrs.size())
        {
            os << "Consequently, the following quantities are unavailable:";
            for (auto& e: errs)
            {
                if (std::find_if(rerrs.begin(), rerrs.end(),
                                 [&](const ErrorList::value_type& r) { return r.first==e.first; } )
                    == rerrs.end())
                {
                    os << " " << e.first;
                }
            }
        }

        throw insight::Exception("%s", os.str().c_str());
    }
}




void supplementedInputDataBase::reportSupplementQuantity(
    const std::string &name,
    ReportedSupplementQuantityValue value,
    const std::string &description,
    const std::string &unit )
{
  boost::mutex::scoped_lock lck(reportedSupplementQuantitiesMutex_);
  reportedSupplementQuantities_.insert( std::make_pair(
        name,
        ReportedSupplementQuantity{ value, description, unit }
    ) );
}


void supplementedInputDataBase::insertSupplementQuantities(
    const std::string &prefix,
    const ReportedSupplementQuantitiesTable &rsqt)
{
    boost::mutex::scoped_lock lck(reportedSupplementQuantitiesMutex_);
    std::transform(
        rsqt.begin(), rsqt.end(),
        std::inserter(reportedSupplementQuantities_, reportedSupplementQuantities_.end()),
        [&](const ReportedSupplementQuantitiesTable::value_type& o)
        {
            auto newname=o.first;
            if (!prefix.empty()) newname=prefix+"/"+newname;
            return ReportedSupplementQuantitiesTable::value_type{
                newname, o.second };
        }
        );
}

const ParameterSet& supplementedInputDataBase::parameters() const
{
    return *parameters_;
}




supplementedInputDataFromParameters::supplementedInputDataFromParameters(
    ParameterSetInput ip,
    const boost::filesystem::path& exePath,
    ActionProgress& ap)
: std::unique_ptr<ParametersBase>(ip.moveParameters()),
    supplementedInputDataBase(std::move(ip), exePath, ap)
{}




const boost::filesystem::path &supplementedInputDataBase::executionPath() const
{
    return executionPath_;
}



supplementedInputDataBase::ReportedSupplementQuantitiesTable
supplementedInputDataBase::reportedSupplementQuantities() const
{
  boost::mutex::scoped_lock lck(reportedSupplementQuantitiesMutex_);
  auto t=reportedSupplementQuantities_;
  for (auto& r: deferredReports_)
      r(t);
  return t;
}



} // namespace insight
