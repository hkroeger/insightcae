
#include "cppextensions.h"


namespace insight
{


ObjectWithBoostSignalConnections::~ObjectWithBoostSignalConnections()
{
    for (const auto& c: connections_)
    {
        c.disconnect();
    }
}

void ObjectWithBoostSignalConnections::disconnectAtEOL(
    const boost::signals2::connection &connection )
{
    connections_.push_back(connection);
}


}
