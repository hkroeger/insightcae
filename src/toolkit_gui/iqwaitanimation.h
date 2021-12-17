#ifndef IQWAITANIMATION_H
#define IQWAITANIMATION_H

#include <QLabel>
#include <QTimer>


class IQWaitAnimation : public QObject
{
    Q_OBJECT

    QLabel* label_;
    QString baseMsg_;
    int nDots_;
    QTimer timer_;

public:
    IQWaitAnimation(const QString& baseMsg, QLabel* out);
    ~IQWaitAnimation();
};


#endif // IQWAITANIMATION_H
