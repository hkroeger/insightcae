#ifndef QTEXTENSIONS_H
#define QTEXTENSIONS_H

#include <QObject>
#include "boost/signals2.hpp"



// helper for boost::signals: disconnect at QObject destruction
void disconnectAtEOL(
    QObject *o,
    const boost::signals2::connection& connection
    );



#endif // QTEXTENSIONS_H
