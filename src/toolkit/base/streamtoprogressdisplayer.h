#ifndef INSIGHT_STREAMTOPROGRESSDISPLAYER_H
#define INSIGHT_STREAMTOPROGRESSDISPLAYER_H

#include "base/streamredirector.h"
#include "base/progressdisplayer.h"

namespace insight {

class StreamToProgressDisplayer
        : public StreamRedirector
{
    ProgressDisplayer& pd_;

public:
    StreamToProgressDisplayer(std::ostream& os, ProgressDisplayer& pd);

protected:
    void processLine( const std::string& line) override;
};

} // namespace insight

#endif // INSIGHT_STREAMTOPROGRESSDISPLAYER_H
