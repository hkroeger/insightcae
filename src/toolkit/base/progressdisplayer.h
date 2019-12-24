#ifndef INSIGHT_PROGRESSDISPLAYER_H
#define INSIGHT_PROGRESSDISPLAYER_H


#include <memory>
#include <map>


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

  ProgressState(
      double t,
      ProgressVariableList pvl,
      const std::string& logMessage = std::string()
      );
};




class ProgressDisplayer
{
public:
    virtual ~ProgressDisplayer();

    virtual void update ( const ProgressState& pi ) =0;

    virtual bool stopRun() const;
};


} // namespace insight

#endif // INSIGHT_PROGRESSDISPLAYER_H
