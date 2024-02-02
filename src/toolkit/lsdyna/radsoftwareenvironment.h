#ifndef INSIGHT_RADSOFTWAREENVIRONMENT_H
#define INSIGHT_RADSOFTWAREENVIRONMENT_H

#include "base/boost_include.h"
#include "base/softwareenvironment.h"
#include "base/outputanalyzer.h"

namespace insight {

class RADSoftwareEnvironment
        : public SoftwareEnvironment
{
public:
    RADSoftwareEnvironment();

    boost::filesystem::path preprocessLSDynaInput(
            const boost::filesystem::path& keyFile,
            int np = 1,
            OutputAnalyzer* oa = nullptr
            );

    void runSolver(
            const boost::filesystem::path& radFile,
            int np = 1,
            OutputAnalyzer* oa = nullptr
            );

    boost::filesystem::path anim2VTK(
            const boost::filesystem::path& animFile
            );

};

} // namespace insight

#endif // INSIGHT_RADSOFTWAREENVIRONMENT_H
