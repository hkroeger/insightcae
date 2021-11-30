#ifndef COMBINEDPROGRESSDISPLAYER_H
#define COMBINEDPROGRESSDISPLAYER_H

#include "base/progressdisplayer.h"
#include <vector>

namespace insight
{


class CombinedProgressDisplayer
    : public ProgressDisplayer
{
public:
    typedef enum {AND, OR} Ops;
protected:
    std::vector<ProgressDisplayer*> displayers_;
    Ops op_;
public:
    CombinedProgressDisplayer ( Ops op = AND );

    void setOp(Ops op);

    void add ( ProgressDisplayer* );

    void update ( const ProgressState& pi ) override;
    void logMessage(const std::string& line) override;
    void setActionProgressValue(const std::string& path, double value) override;
    void setMessageText(const std::string& path, const std::string& message) override;
    void finishActionProgress(const std::string& path) override;
    void reset() override;

    bool stopRun() const override;
};


}

#endif // COMBINEDPROGRESSDISPLAYER_H
