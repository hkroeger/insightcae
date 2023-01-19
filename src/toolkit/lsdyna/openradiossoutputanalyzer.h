#ifndef INSIGHT_OPENRADIOSSOUTPUTANALYZER_H
#define INSIGHT_OPENRADIOSSOUTPUTANALYZER_H

#include "base/outputanalyzer.h"
#include "base/progressdisplayer.h"

#include "base/boost_include.h"

namespace insight {

class OpenRadiossOutputAnalyzer
: public OutputAnalyzer
{
public:
    typedef std::function<void(std::string,int)> AnimWriteCallback;

private:
    boost::regex progressLinePattern_, timePattern_, animWritePattern_, animName_;
    double endTime_;
    std::shared_ptr<ActionProgress> progr_;

    std::unique_ptr<ProgressState> curPS_;

    AnimWriteCallback animWriteCallback_;

public:
    OpenRadiossOutputAnalyzer(
            double endTime,
            ProgressDisplayer* parentProgress=nullptr,
            AnimWriteCallback animWriteCallback = AnimWriteCallback() );

    void update(const std::string& line) override;
};

} // namespace insight

#endif // INSIGHT_OPENRADIOSSOUTPUTANALYZER_H
