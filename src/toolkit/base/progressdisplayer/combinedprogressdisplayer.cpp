#include "combinedprogressdisplayer.h"

using namespace std;

namespace insight
{



CombinedProgressDisplayer::CombinedProgressDisplayer ( CombinedProgressDisplayer::Ops op )
    : op_ ( op )
{}


void CombinedProgressDisplayer::add ( ProgressDisplayer* d )
{
    displayers_.push_back ( d );
}

void CombinedProgressDisplayer::update ( const ProgressState& pi )
{
    for (auto* d: displayers_)
    {
        d->update ( pi );
    }
}

void CombinedProgressDisplayer::setActionProgressValue(const string &path, double value)
{
  for (auto* d: displayers_)
  {
      d->setActionProgressValue (path, value );
  }
}

void CombinedProgressDisplayer::setMessageText(const string &path, const string &message)
{
  for (auto* d: displayers_)
  {
      d->setMessageText ( path, message );
  }
}

void CombinedProgressDisplayer::finishActionProgress(const string &path)
{
  for (auto* d: displayers_)
  {
      d->finishActionProgress ( path );
  }
}

void CombinedProgressDisplayer::reset()
{
  for (auto* d: displayers_)
  {
      d->reset();
  }
}


bool CombinedProgressDisplayer::stopRun() const
{
    bool stop=false;

    if ( ( op_==AND ) && ( displayers_.size() >0 ) ) {
        stop=true;
    }

    for ( const ProgressDisplayer* d: displayers_ ) {
        if ( op_==AND ) {
            stop = stop && d->stopRun();
        } else if ( op_==OR ) {
            stop = stop || d->stopRun();
        }
    }
    return stop;
}



}
