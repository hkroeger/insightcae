
#include "cppextensions.h"
#include "boost/signals2/shared_connection_block.hpp"


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
