#include "streamtoprogressdisplayer.h"

namespace insight {


StreamToProgressDisplayer::StreamToProgressDisplayer(std::ostream &os, ProgressDisplayer &pd)
    : StreamRedirector(os),
      pd_(pd)
{}

void StreamToProgressDisplayer::processLine(const std::string &line)
{
    std::cerr<<"process line:" <<line<<std::endl;
    pd_.logMessage( line );
}


} // namespace insight
