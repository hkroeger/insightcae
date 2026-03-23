#ifndef ACTIONPROGRESS_H
#define ACTIONPROGRESS_H


#include <set>

#include "boost/signals2.hpp"
#include "boost/smart_ptr/enable_shared_from_this.hpp"

#include "base/factory.h"
#include "base/elementpath.h"

namespace insight {

class ProgressDisplayer;
class ActionProgress;

typedef std::shared_ptr<ActionProgress> ActionProgressPtr;

class ActionProgress
    : public std::enable_shared_from_this<ActionProgress>
{
    friend class ProgressDisplayer;

public:
    typedef boost::signals2::signal<void()> StopSignal;

protected:
    // action status
    boost::variant<ProgressDisplayer*,ActionProgressPtr> parent_;
    std::set<ActionProgress*> children_;

    std::string name_; ///< label of this action
    double ci_=0.; ///< current action index
    double maxi_=1.; ///< final target action index
    StopSignal stopSignal_; ///< emitted when this action is to be stopped prematurely

    bool skipFinishOnDestroy_ = false;

    std::string actionPath() const;
    ProgressDisplayer& parentDisplayer();
    ActionProgress* getChildAction(
        const std::string& pathstr );

    void startAction();

    ActionProgress(
        ProgressDisplayer* parent,
        std::string name,
        double nSteps );

    ActionProgress(
        ActionProgressPtr parentAction,
        std::string name,
        double nSteps );

public:
    virtual ~ActionProgress();

    ActionProgressPtr forkNewAction(
        double nSteps, const std::string& name = "Overall" );

    const std::string& name() const;

    void stepUp(double steps=1);
    void stepUp(const std::string& msg, double steps=1);
    void stepTo(double i);
    void completed();
    void operator++();
    void operator+=(double n);
    void message(const std::string& message);

    void setNSteps( int nSteps );

    StopSignal& stopSignal( );

    void triggerStop();

    bool isStoppable();

    bool stopIsDemanded() const;
};




} // namespace insight

#endif // ACTIONPROGRESS_H
