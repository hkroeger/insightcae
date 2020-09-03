#ifndef INSIGHT_PROGRESSDISPLAYER_H
#define INSIGHT_PROGRESSDISPLAYER_H


#include <memory>
#include <map>
#include <vector>


namespace insight {

class ProgressDisplayer;




typedef
  std::shared_ptr<ProgressDisplayer>
  ProgressDisplayerPtr;

typedef
  std::map<std::string, double>
  ProgressVariableList;





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


//class ProgressDisplayer;


//class ActionProgress
//{
//  friend class ProgressDisplayer;

//  ProgressDisplayer& dispatcher_;
//  std::string path_;
//  double ci_, maxi_;

//  ActionProgress(ProgressDisplayer& dispatcher, std::string path, double nSteps);

//public:
//  ActionProgress(const ActionProgress& o);
//  ~ActionProgress();

//  // ====================================================================================
//  // ======== other action status reporting

//  ActionProgress forkNewAction(const std::string& name, double nSteps) const;
//  void stepUp(double steps=1);
//  void stepTo(double i);
//  void operator++();
//  void operator+=(double n);
//  void message(const std::string& message) const;

//  void update(const ProgressState& pi) const;
//  bool stopRun() const;
//};


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
  ~ActionProgress();

  void setActionProgressValue(const std::string &path, double value) override;
  void setMessageText(const std::string &path, const std::string& message) override;
  void finishActionProgress(const std::string &path) override;

  void update ( const ProgressState& pi ) override;
  void reset() override;
};


} // namespace insight

#endif // INSIGHT_PROGRESSDISPLAYER_H
