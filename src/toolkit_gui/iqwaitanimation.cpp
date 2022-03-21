#include "iqwaitanimation.h"
#include "base/exception.h"

const int nUpdatesPerCycle = 200;


IQWaitAnimation::IQWaitAnimation(QProgressBar* pb, int msecCycle)
    : progressBar_(pb),
      i_(0)
{
    insight::dbg()<<"WaitingAnimation"<<std::endl;
    progressBar_->setRange(0, nUpdatesPerCycle);
    timer_.setInterval(msecCycle/nUpdatesPerCycle);
    connect(&timer_, &QTimer::timeout,
            this, [&]()
    {
        ++i_;
        progressBar_->setValue(i_);
        if (i_>nUpdatesPerCycle) i_=0;
    }
    );
    timer_.start();
}




IQWaitAnimation::~IQWaitAnimation()
{
    timer_.stop();
    progressBar_->setValue(nUpdatesPerCycle);
}

