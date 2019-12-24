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
    for ( ProgressDisplayer* d: displayers_ ) {
        d->update ( pi );
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
