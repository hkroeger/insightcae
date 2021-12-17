#include "iqwaitanimation.h"

#include "base/exception.h"


IQWaitAnimation::IQWaitAnimation(const QString& baseMsg, QLabel* out)
    : label_(out),
      baseMsg_(baseMsg),
      nDots_(0)
{
    insight::dbg()<<"WaitingAnimation"<<std::endl;
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout,
            this, [&]()
    {
        ++nDots_;
        label_->setText(baseMsg_ + QString(nDots_, '.'));
        if (nDots_>15)
            nDots_=0;
    }
    );
    timer_.start();
}




IQWaitAnimation::~IQWaitAnimation()
{
    timer_.stop();
    label_->setText(baseMsg_);
}

