#ifndef TEXTPROGRESSDISPLAYER_H
#define TEXTPROGRESSDISPLAYER_H

#include "base/progressdisplayer.h"

namespace insight
{

class TextProgressDisplayer
    : public ProgressDisplayer
{
public:
    virtual void update ( const ProgressState& pi );
};

}

#endif // TEXTPROGRESSDISPLAYER_H
