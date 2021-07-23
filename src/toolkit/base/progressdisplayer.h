#ifndef INSIGHT_PROGRESSDISPLAYER_H
#define INSIGHT_PROGRESSDISPLAYER_H


#include <memory>
#include <map>
#include <vector>
#include <string>


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




class ActionProgress;




class ProgressDisplayer
{
  friend class ActionProgress;

protected:
    // action status
    double ci_=0., maxi_=1.;

    // ====================================================================================
    // ======== to be used internally only:


    virtual std::string actionPath() const;

public:
    virtual ~ProgressDisplayer();

    // ====================================================================================
    // ======== convergence state reporting

    virtual void update ( const ProgressState& pi ) =0;

    // ====================================================================================
    // ======== action progress reporting
    ActionProgress forkNewAction(double nSteps, const std::string& name="Overall" );
    void stepUp(double steps=1);
    void stepTo(double i);
    void completed();
    void operator++();
    void operator+=(double n);
    void message(const std::string& message);


    // ====================================================================================
    // ======== action status reporting
    virtual void setActionProgressValue(const std::string &path, double value) =0;
    virtual void setMessageText(const std::string &path, const std::string& message) =0;
    virtual void finishActionProgress(const std::string &path) =0;

    virtual void reset() =0;
    virtual bool stopRun() const;
};




class ActionProgress
    : public ProgressDisplayer
{

protected:
  ProgressDisplayer& parentAction_;
  std::string name_;


  std::string actionPath() const override;

public:
  ActionProgress(const ProgressDisplayer& parentAction, std::string path, double nSteps);
  virtual ~ActionProgress();

  void update ( const ProgressState& pi ) override;

  void setActionProgressValue(const std::string &path, double value) override;
  void setMessageText(const std::string &path, const std::string& message) override;
  void finishActionProgress(const std::string &path) override;

  void reset() override;
};




} // namespace insight

#endif // INSIGHT_PROGRESSDISPLAYER_H
