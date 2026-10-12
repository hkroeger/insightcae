#ifndef IQSIMULATIONTIMEDISPLAY_H
#define IQSIMULATIONTIMEDISPLAY_H

#include "toolkit_gui_export.h"

#include <QLabel>

#include "base/progressdisplayer.h"


/**
 * @brief The IQSimulationTimeDisplay class
 * displays the current simulation time,
 * as reported through the progress displayer (e.g. extracted from the solver output).
 * All other progress information is ignored.
 */
class TOOLKIT_GUI_EXPORT IQSimulationTimeDisplay
    : public QLabel,
      public insight::ProgressDisplayer
{
    Q_OBJECT

    void showTime(double t);

public:
    IQSimulationTimeDisplay(QWidget* parent = nullptr);

    void update(const insight::ProgressState& pi) override;
    void logMessage(const std::string& line) override;
    void setActionProgressValue(const std::string& path, double value) override;
    void setMessageText(const std::string& path, const std::string& message) override;
    void finishActionProgress(const std::string& path) override;
    void reset() override;
};


#endif // IQSIMULATIONTIMEDISPLAY_H
