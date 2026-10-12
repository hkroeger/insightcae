#include "iqsimulationtimedisplay.h"

#include "base/translations.h"

#include <cmath>



void IQSimulationTimeDisplay::showTime(double t)
{
    if (std::isnan(t))
    {
        // no time yet: en dash (kept out of the translated string, sources are ASCII)
        setText(QString(_("t = %1")).arg(QChar(0x2013)));
    }
    else
    {
        setText(QString(_("t = %1")).arg(t, 0, 'g', 6));
    }
}




IQSimulationTimeDisplay::IQSimulationTimeDisplay(QWidget* parent)
    : QLabel(parent)
{
    setAlignment(Qt::AlignCenter);
    setToolTip(_("Current simulation time, as reported in the solver output"));
    setTextInteractionFlags(Qt::TextSelectableByMouse);
    showTime(std::nan(""));
}




// called from different threads: post into GUI thread.
// (the context object drops pending calls, if this widget is deleted)

void IQSimulationTimeDisplay::update(const insight::ProgressState& pi)
{
    double t=pi.first;
    if (std::isnan(t)) return;

    QMetaObject::invokeMethod(
        this, [this,t]() { showTime(t); },
        Qt::QueuedConnection );
}


void IQSimulationTimeDisplay::reset()
{
    QMetaObject::invokeMethod(
        this, [this]() { showTime(std::nan("")); },
        Qt::QueuedConnection );
}


void IQSimulationTimeDisplay::logMessage(const std::string&)
{}

void IQSimulationTimeDisplay::setActionProgressValue(const std::string&, double)
{}

void IQSimulationTimeDisplay::setMessageText(const std::string&, const std::string&)
{}

void IQSimulationTimeDisplay::finishActionProgress(const std::string&)
{}
