#include "streamtoprogressdisplayer.h"

namespace insight {


StreamToProgressDisplayer::StreamToProgressDisplayer(std::ostream &os, ProgressDisplayer &pd)
    : StreamRedirector(os),
      pd_(pd)
{}

void StreamToProgressDisplayer::processLine(const std::string &line)
{
    pd_.logMessage( line );
}


} // namespace insight
