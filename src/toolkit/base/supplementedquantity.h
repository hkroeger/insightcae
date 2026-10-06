#ifndef INSIGHT_SUPPLEMENTEDQUANTITY_H
#define INSIGHT_SUPPLEMENTEDQUANTITY_H

#include "toolkit_export.h"

#include <memory>
#include <string>
#include <functional>
#include <type_traits>
#include <utility>

#include "boost/optional.hpp"
#include "boost/thread.hpp"

#include "base/exception.h"
#include "base/actionprogress.h"

namespace insight {




/**
 * @brief describeException
 * @return
 * a one-line message of the exception stored in ex
 */
std::string describeException(std::exception_ptr ex);




/**
 * @brief The SupplementedQuantityError class
 * is thrown on access to a deferred quantity, whose computation failed.
 * Refers to the quantity, in which the error originally occurred
 * (might be different from the accessed quantity, if the accessed quantity
 * depends on a failed one).
 */
class TOOLKIT_EXPORT SupplementedQuantityError
    : public Exception
{
    std::string quantity_, rootQuantity_, rootMessage_;
    std::exception_ptr rootCause_;

public:
    SupplementedQuantityError(
        const std::string& quantity,
        std::exception_ptr cause );

    /// name of the quantity which was accessed
    const std::string& quantity() const;
    /// name of the quantity, in which the error occurred originally
    const std::string& rootQuantity() const;
    /// error message of the original error
    const std::string& rootMessage() const;
    /// the original exception
    std::exception_ptr rootCause() const;
    /// true, if the error occurred not in the computation of this quantity but in a dependency
    bool isDependencyFailure() const;
};




/**
 * @brief The SupplementedQuantityBase class
 * is the type-independent part of a deferred computation in a supplementedInputData object.
 *
 * The computation is executed at most once, either
 * - by a worker thread, which was launched to compute all quantities in parallel, or
 * - on demand, in the thread which first accesses the value.
 * Concurrent accesses during computation wait for the result.
 * An exception during computation is stored and rethrown on every access.
 */
class TOOLKIT_EXPORT SupplementedQuantityBase
{
public:
    enum Status { Pending, Running, Done, Failed };

    /**
     * creates a progress object for the computation of the named quantity.
     * Provided by the owning supplementedInputData.
     */
    typedef std::function<ActionProgressPtr(const std::string&)> ProgressSource;

    /**
     * receives the warnings, which are issued during the computation of the named quantity.
     * Provided by the owning supplementedInputData.
     */
    typedef std::function<void(const std::string& quantity, const std::string& message)> IssueSink;

private:
    std::string name_;
    ProgressSource progressSource_;
    IssueSink issueSink_;

    mutable boost::mutex mtx_;
    mutable boost::condition_variable cv_;
    Status status_;
    boost::thread::id runningIn_;
    std::exception_ptr error_;

    /**
     * claims the computation for the calling thread.
     * Returns false, if computation was already started.
     */
    bool claim();

    void execute();

protected:
    virtual void compute(ActionProgress& ap) =0;

    /**
     * throws a SupplementedQuantityError, if failed
     */
    void ensureComputed() const;

public:
    SupplementedQuantityBase(
        const std::string& name,
        ProgressSource progressSource,
        IssueSink issueSink = IssueSink() );

    virtual ~SupplementedQuantityBase();

    const std::string& name() const;
    Status status() const;

    /**
     * @brief error
     * @return
     * the stored exception, if failed. Null otherwise.
     */
    std::exception_ptr error() const;

    /**
     * @brief tryRun
     * computes the quantity in the calling thread, if not done or running yet.
     * Never throws (except boost::thread_interrupted).
     * @return
     * true, if the computation was executed by this call
     */
    bool tryRun();

    /**
     * @brief wait
     * blocks until computation has finished (successful or not).
     * Computes in the calling thread, if not started yet. Never throws
     * (except boost::thread_interrupted).
     */
    void wait() const;
};


typedef std::shared_ptr<SupplementedQuantityBase> SupplementedQuantityBasePtr;




template<class T>
class SupplementedQuantity
    : public SupplementedQuantityBase
{
    std::function<T(ActionProgress&)> function_;
    boost::optional<T> value_;

protected:
    void compute(ActionProgress& ap) override
    {
        value_ = function_(ap);
        function_ = nullptr; // release captures
    }

public:
    SupplementedQuantity(
        const std::string& name,
        ProgressSource progressSource,
        IssueSink issueSink,
        std::function<T(ActionProgress&)> function )
        : SupplementedQuantityBase(name, progressSource, issueSink),
        function_(function)
    {}

    const T& get() const
    {
        ensureComputed();
        return *value_;
    }
};




/**
 * dereferencing behaviour of Supplemented<T>:
 * the value itself for plain types, the pointee for smart pointers.
 * Thus, existing code like "*sp().blocking" or "sp().feature_->..."
 * keeps its meaning, when a shared_ptr member is converted into a Supplemented one.
 */
template<class T>
struct SupplementedDereference
{
    typedef const T& reference;
    typedef const T* pointer;
    static reference deref(const T& v) { return v; }
    static pointer arrow(const T& v) { return &v; }
};

template<class E>
struct SupplementedDereference<std::shared_ptr<E> >
{
    typedef E& reference;
    typedef E* pointer;
    static reference deref(const std::shared_ptr<E>& v)
    {
        if (!v) throw insight::Exception("dereferencing null supplemented quantity");
        return *v;
    }
    static pointer arrow(const std::shared_ptr<E>& v) { return v.get(); }
};




/**
 * @brief The Supplemented class
 * is a handle to a deferred quantity of type T.
 * It is intended to be used as a member of supplementedInputData classes
 * in place of plain-typed members.
 * Values are assigned in the constructor of the supplementedInputData by
 *
 *   myQuantity_ = deferred("myQuantity", [this]() { ...; return value; } );
 *
 * Other deferred computations may access the value. Then they wait for,
 * or trigger, the computation.
 *
 * Accessing a value, whose computation failed, throws a SupplementedQuantityError.
 *
 * Access:
 * - implicit conversion to const T& (e.g. arithmetic: 2.*sp().L_)
 * - get(): const T&
 * - operator*, operator->: the value or, for shared_ptr, the pointee
 * - operator() is forwarded (e.g. element access sp().L_(0) for matrices)
 * - begin()/end() for containers (range-based for loops)
 */
template<class T>
class Supplemented
{
    std::shared_ptr<const SupplementedQuantity<T> > quantity_;
    std::shared_ptr<const T> value_; // for directly assigned values

public:
    typedef T value_type;

    Supplemented() = default;

    /**
     * a quantity with an immediately known value
     */
    Supplemented(const T& value)
        : value_(std::make_shared<T>(value))
    {}

    Supplemented(std::shared_ptr<const SupplementedQuantity<T> > q)
        : quantity_(q)
    {}

    bool isDefined() const
    {
        return bool(quantity_) || bool(value_);
    }

    /**
     * @brief ok
     * computes, if needed, but never throws (except boost::thread_interrupted)
     * @return
     * true, if a value is available.
     */
    bool ok() const
    {
        if (value_) return true;
        if (!quantity_) return false;
        quantity_->wait();
        return quantity_->status()==SupplementedQuantityBase::Done;
    }

    /**
     * @brief status
     * non-blocking
     */
    SupplementedQuantityBase::Status status() const
    {
        if (value_) return SupplementedQuantityBase::Done;
        if (!quantity_) return SupplementedQuantityBase::Failed;
        return quantity_->status();
    }

    /**
     * @brief error
     * non-blocking
     * @return
     * the error, if the computation failed. Null otherwise.
     */
    std::exception_ptr error() const
    {
        if (quantity_)
            return quantity_->error();
        if (!value_)
            return std::make_exception_ptr(
                insight::Exception("undefined supplemented quantity") );
        return nullptr;
    }

    const T& get() const
    {
        if (value_) return *value_;
        if (!quantity_)
            throw insight::Exception("access to undefined supplemented quantity");
        return quantity_->get();
    }

    typename SupplementedDereference<T>::reference operator*() const
    { return SupplementedDereference<T>::deref(get()); }

    typename SupplementedDereference<T>::pointer operator->() const
    { return SupplementedDereference<T>::arrow(get()); }

    operator const T&() const { return get(); }

    // forward call operator, e.g. element access of matrices: sp().L_(0)
    template<class... Args>
    auto operator()(Args&&... args) const
        -> decltype(std::declval<const T&>()(std::forward<Args>(args)...))
    { return get()(std::forward<Args>(args)...); }

    // allow range-based for loops over containers
    template<class TT=T> auto begin() const -> decltype(std::declval<const TT&>().begin())
    { return get().begin(); }
    template<class TT=T> auto end() const -> decltype(std::declval<const TT&>().end())
    { return get().end(); }
};




} // namespace insight

#endif // INSIGHT_SUPPLEMENTEDQUANTITY_H
