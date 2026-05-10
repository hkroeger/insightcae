#ifndef INSIGHT_PROGRESSDISPLAYER_H
#define INSIGHT_PROGRESSDISPLAYER_H


#include <memory>
#include <map>
#include <vector>
#include <string>
#include <set>
#include <functional>
#include <mutex>

#include "boost/optional.hpp"
#include "base/actionprogress.h"


namespace insight {

class ProgressDisplayer;




typedef
  std::shared_ptr<ProgressDisplayer>
  ProgressDisplayerPtr;

typedef
  std::map<std::string, double>
  ProgressVariableList;




/**
 * @brief The ProgressState struct represents a change in progress of some action.
 * It is marked by a single number (e.g. a time value) and can have some additional properties:
 * a set of numbers (maybe residuals) and a log message.
 */
struct ProgressState
    : public std::pair<double, ProgressVariableList>
{
  std::string logMessage_;

  ProgressState(); // required for qMetaType registration
  ProgressState(
      double t,
      ProgressVariableList pvl,
      const std::string& logMessage = std::string()
      );
};




typedef std::shared_ptr<ProgressState> ProgressStatePtr;
typedef std::vector<ProgressStatePtr> ProgressStatePtrList;






class ProgressDisplayer
{
    friend class ActionProgress;

protected:
    bool stopTriggered_=false;

    mutable std::mutex childActionsMutex_;
    std::set<ActionProgress*> childActions_;

public:
    ProgressDisplayer();
    virtual ~ProgressDisplayer();

    /**
     * @brief forkNewAction
     * Create a new top-level ActionProgress directly under this displayer.
     * Multiple calls produce independent sibling actions, all traversable
     * via findAction().
     */
    ActionProgressPtr forkNewAction(double nSteps, const std::string& name);


    // ====================================================================================
    // ======== action status reporting
    /**
     * @brief setActionProgressValue
     * implementations must be safe to be called from different thread!
     * @param path
     * @param value
     * progress indicator between 0 and 1
     */
    virtual void setActionProgressValue(const std::string &path, double value) =0;
    /**
     * @brief setMessageText
     * implementations must be safe to be called from different thread!
     * @param path
     * @param message
     */
    virtual void setMessageText(const std::string &path, const std::string& message) =0;
    /**
     * @brief finishActionProgress
     * implementations must be safe to be called from different thread!
     * @param path
     */
    virtual void finishActionProgress(const std::string &path) =0;

    /**
     * @brief reset
     * implementations must be safe to be called from different thread!
     */
    virtual void reset() =0;

    // ====================================================================================
    // ======== convergence state reporting

    /**
     * @brief update
     * implementations must be safe to be called from different thread!
     * @param pi
     */
    virtual void update ( const ProgressState& pi ) =0;

    // ====================================================================================
    // ======== log message
    /**
     * @brief logMessage
     * implementations must be safe to be called from different thread!
     * @param line
     */
    virtual void logMessage(const std::string& line) =0;

    /**
     * @brief findAction
     * Traverse the action hierarchy to find the ActionProgress identified by
     * @p path (action names concatenated with "/", rooted at overallAction()).
     */
    ActionProgress* findAction(const std::string& path);

    void triggerStop(const std::string& actionPath);

    virtual bool stopIsDemanded() const;

    // ActionProgress methods forwarded via overallAction() for convenience:
    // use pd.overallAction().forkNewAction(...) / pd.overallAction().message(...) etc.
};




} // namespace insight


#endif // INSIGHT_PROGRESSDISPLAYER_H
