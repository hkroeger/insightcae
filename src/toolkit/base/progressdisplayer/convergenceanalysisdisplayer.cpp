#include "convergenceanalysisdisplayer.h"

#include "base/linearalgebra.h"

#include <cmath>
#include <iostream>

using namespace std;

namespace insight
{


ConvergenceAnalysisDisplayer::ConvergenceAnalysisDisplayer (
    const std::string& progvar,
    const Parameters& p )
    : progvar_ ( progvar ),
      p_ ( p ),
      cbv_ ( std::make_unique<ConvergenceByVariance>(p.fraction) ),
      converged_ ( false )
{}


ConvergenceAnalysisDisplayer::~ConvergenceAnalysisDisplayer()
{}


const ConvergenceAnalysisDisplayer::Parameters&
ConvergenceAnalysisDisplayer::parameters() const
{
    return p_;
}


void ConvergenceAnalysisDisplayer::update ( const ProgressState& pi )
{
    auto pv = pi.second.find ( progvar_ );

    if ( pv == pi.second.end() )
        return;

    double cov = arma::as_scalar( (*cbv_)( arma::rowvec{ pv->second } ) );
    times_.push_back( pi.first );
    auto window = cbv_->windowSize();
    auto n = times_.size();

    if ( n < 2 )
        return; // no variance yet

    double span = times_.back() - times_[ n - std::min<size_t>(window, n) ];

    std::cout<<"CoV of "<<progvar_<<" = "<<cov
             <<" (trailing window "<<window<<" samples, time span "<<span<<")";

    bool windowOk =
        p_.minWindowUnit == Parameters::Samples ?
            ( double(window) >= p_.minWindow ) :
            ( window>=2 && span >= p_.minWindow );

    // current state (not latched): when several displayers are combined
    // with AND, all variables have to be converged at the same time
    converged_ = windowOk && ( cov<p_.threshold );

    if ( !windowOk )
    {
        std::cout<<", not converged (window ";
        if ( p_.minWindowUnit == Parameters::Samples )
            std::cout<<"< "<<p_.minWindow<<" samples)";
        else
            std::cout<<"time span < "<<p_.minWindow<<")";
        std::cout<<std::endl;
    }
    else if ( converged_ )
        std::cout<<" >>> CONVERGED"<<std::endl;
    else
        std::cout<<", not converged"<<std::endl;
}

void ConvergenceAnalysisDisplayer::logMessage(const std::string &)
{}

void ConvergenceAnalysisDisplayer::setActionProgressValue(const string &, double)
{}

void ConvergenceAnalysisDisplayer::setMessageText(const string &, const string &)
{}

void ConvergenceAnalysisDisplayer::finishActionProgress(const string &)
{}

void ConvergenceAnalysisDisplayer::reset()
{}

bool ConvergenceAnalysisDisplayer::stopIsDemanded() const
{
    return converged_;
}


}
