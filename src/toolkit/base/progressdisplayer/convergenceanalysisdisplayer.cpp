#include "convergenceanalysisdisplayer.h"

#include <cmath>
#include <iostream>

using namespace std;

namespace insight
{


ConvergenceAnalysisDisplayer::ConvergenceAnalysisDisplayer ( const std::string& progvar, double threshold )
    : progvar_ ( progvar ),
      istart_ ( 10 ),
      co_ ( 15 ),
      threshold_ ( threshold ),
      converged_ ( false )
{}

void ConvergenceAnalysisDisplayer::update ( const ProgressState& pi )
{
    decltype ( pi.second ) ::const_iterator pv=pi.second.find ( progvar_ );

    if ( pv != pi.second.end() ) {
        trackedValues_.push_back ( pv->second );
    }

    if ( trackedValues_.size() > istart_ ) {
        std::vector<double> ym;
        for ( size_t i=istart_; i<trackedValues_.size(); i++ ) {
            size_t i0=i/2;
            double sum=0.0;
            for ( size_t j=i0; j<i; j++ ) {
                sum+=trackedValues_[j];
            }
            ym.push_back ( sum/double ( i-i0 ) );
        }

        if ( ym.size() >co_ ) {
            double maxrely=0.0;
            for ( size_t j=ym.size()-1; j>=ym.size()-co_; j-- ) {
                double rely=fabs ( ym[j]-ym[j-1] ) / ( fabs ( ym[j] )+1e-10 );
                maxrely=std::max ( rely, maxrely );
            }

            std::cout<<"max rel. change of "<<progvar_<<" = "<<maxrely;

            if ( maxrely<threshold_ ) {
                std::cout<<" >>> CONVERGED"<<std::endl;
                converged_=true;
            } else {
                std::cout<<", not converged"<<std::endl;
            }
        }
    }
}

void ConvergenceAnalysisDisplayer::setActionProgressValue(const string &, double)
{}

void ConvergenceAnalysisDisplayer::setMessageText(const string &, const string &)
{}

void ConvergenceAnalysisDisplayer::finishActionProgress(const string &)
{}

void ConvergenceAnalysisDisplayer::reset()
{}

bool ConvergenceAnalysisDisplayer::stopRun() const
{
    return converged_;
}


}
