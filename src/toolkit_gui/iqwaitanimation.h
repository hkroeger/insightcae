#ifndef IQWAITANIMATION_H
#define IQWAITANIMATION_H

#include <QProgressBar>
#include <QTimer>


class IQWaitAnimation : public QObject
{
    Q_OBJECT

    QProgressBar* progressBar_;
    QTimer timer_;
    int i_;

public:
    IQWaitAnimation(QProgressBar* out, int msecCycle);
    ~IQWaitAnimation();
};


#endif // IQWAITANIMATION_H
