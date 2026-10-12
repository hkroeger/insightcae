#ifndef IQREMOTESERVERSTATUS_H
#define IQREMOTESERVERSTATUS_H

#include "toolkit_gui_export.h"

#include <QIcon>
#include <QString>

#include <functional>

#include "base/remoteserver.h"

class QObject;


/**
 * display and (background) evaluation of the state of remote servers,
 * shared by the dialogs, which list servers
 */
namespace IQRemoteServerStatus
{

enum class State { Unknown, Online, Offline, Busy };

/**
 * @brief icon
 * small status circle: dotted grey (unknown), green (online),
 * empty (offline) or a pie (busy; yellow, orange >50%, red >75%)
 * @param utilization
 * only for Busy: processor utilization (0...1)
 */
TOOLKIT_GUI_EXPORT QIcon icon(State state, double utilization = 0.);

/**
 * @brief isBusy
 * some core is fully occupied or more than half of the capacity is used
 */
TOOLKIT_GUI_EXPORT bool isBusy(const insight::RemoteServer::Config::ProcessorLoad& load);

/**
 * @brief describe
 * e.g. "online, busy: 3 of 16 cores fully occupied, 19% utilization"
 */
TOOLKIT_GUI_EXPORT QString describe(const insight::RemoteServer::Config::ProcessorLoad& load);

/**
 * @brief checkInBackground
 * check the reachability and then the processor load of a server
 * in a background thread. The callbacks are executed in the GUI thread,
 * as long as the receiver exists.
 * @param onLoad
 * only called for online servers, if the load could be determined
 */
TOOLKIT_GUI_EXPORT void checkInBackground(
    insight::RemoteServer::ConfigPtr cfg,
    QObject* receiver,
    std::function<void(bool online)> onReachability,
    std::function<void(const insight::RemoteServer::Config::ProcessorLoad&)> onLoad );

}


#endif // IQREMOTESERVERSTATUS_H
