#ifndef PARAVIEW_H
#define PARAVIEW_H


#include "base/boost_include.h"
#include "base/tools.h"
#include "base/externalprocess.h"

#include <memory>


namespace insight
{

class Paraview : public ExternalProcess
{
    const boost::filesystem::path& caseDirectory_, dataDirectory_;
    std::unique_ptr<TemporaryFile> loadScript_;

    std::ostream& createLoadScript();

    std::string caseLabel_;

public:
    Paraview(
            const boost::filesystem::path& caseDirectory,
            const boost::filesystem::path& stateFile,
            bool batch=false,
            bool parallelProjection=false,
            bool rescale=false,
            bool onlyLatestTime=true,
            double fromTime=0, double toTime=1e10,
            const std::vector<std::string> &additionalClientArgs = {},
            const boost::filesystem::path& dataDirectory = "",
            const std::string& caseLabel=""
            );
};

}


#endif // PARAVIEW_H
