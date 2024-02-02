#include "qtextensions.h"


void disconnectAtEOL(QObject *o, const boost::signals2::connection &connection)
{
    QObject::connect(
        o, &QObject::destroyed, o,
            [connection]()
            {
              connection.disconnect();
            }
        );
}
