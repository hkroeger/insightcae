#ifndef COMBINEDPROGRESSDISPLAYER_H
#define COMBINEDPROGRESSDISPLAYER_H

#include "base/progressdisplayer.h"
#include <vector>

namespace insight
{


class CombinedProgressDisplayer
    : public ProgressDisplayer
{
public:
    typedef enum {AND, OR} Ops;
protected:
    std::vector<ProgressDisplayer*> displayers_;
    Ops op_;
public:
    CombinedProgressDisplayer ( Ops op );
    void add ( ProgressDisplayer* );
    virtual void update ( const ProgressState& pi );
    virtual bool stopRun() const;
};


}

#endif // COMBINEDPROGRESSDISPLAYER_H
