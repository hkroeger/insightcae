#ifndef INSIGHT_PROGRESSDISPLAYER_H
#define INSIGHT_PROGRESSDISPLAYER_H


#include <memory>
#include <map>


namespace insight {

class ProgressDisplayer;




typedef std::shared_ptr<ProgressDisplayer> ProgressDisplayerPtr;
typedef std::map<std::string, double> ProgressVariableList;
typedef std::pair<double, ProgressVariableList> ProgressState;




class ProgressDisplayer
{
public:
    virtual ~ProgressDisplayer();

    virtual void update ( const ProgressState& pi ) =0;

    virtual bool stopRun() const;
};


} // namespace insight

#endif // INSIGHT_PROGRESSDISPLAYER_H
