
#include "cppextensions.h"
#include "base/exception.h"
#include "boost/signals2/shared_connection_block.hpp"

namespace std
{


void observer_ptr_base::register_at_observable()
{
    if (valid())
        observed_->register_observer(this);
}


void observer_ptr_base::unregister_at_observable()
{
    if (valid())
        observed_->unregister_observer(this);
}

observer_ptr_base::observer_ptr_base()
    : observed_(nullptr)
{}


observer_ptr_base::observer_ptr_base(const observer_ptr_base &o)
    : observed_(o.observed_)
{
    register_at_observable();
}



observer_ptr_base::observer_ptr_base(observable *o)
    : observed_(o)
{
    register_at_observable();
}




observer_ptr_base::~observer_ptr_base()
{
    unregister_at_observable();
}



void observer_ptr_base::operator=(const observer_ptr_base& o)
{
    unregister_at_observable();
    observed_=o.observed_;
    register_at_observable();
}



invalid_observer_ptr::invalid_observer_ptr()
    : insight::Exception("attempt to access an invalid pointer")
{}




void observable::register_observer(observer_ptr_base* obs)
{
    observers_.insert(obs);
}




void observable::unregister_observer(observer_ptr_base* obs)
{
    observers_.erase(obs);
}




observable::~observable()
{
    auto obs=observers_; // loop over copy, since observers will remove itself and modify list
    for (auto& o: obs)
    {
        o->invalidate();
    }
}




}



namespace insight
{


ObjectWithBoostSignalConnections::~ObjectWithBoostSignalConnections()
{
    for (const auto& c: connections_)
    {
        c.disconnect();
    }
}

const boost::signals2::connection &
ObjectWithBoostSignalConnections::disconnectAtEOL(
    const boost::signals2::connection &connection )
{
    connections_.push_back(connection);
    return connection;
}

std::vector<boost::signals2::shared_connection_block>
ObjectWithBoostSignalConnections::block_all()
{
    std::vector<boost::signals2::shared_connection_block> blc;
    for (const auto& c: connections_)
    {
        blc.push_back(boost::signals2::shared_connection_block(c));
    }
    return blc;
}


}
