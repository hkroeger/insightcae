#ifndef READPOSTPROCESSINGFILES_H
#define READPOSTPROCESSINGFILES_H

#include "openfoam/openfoamcase.h"
#include "base/intervals.h"


namespace insight {



class TabularInterval
    : public Interval
{
    arma::mat table_;
public:
    TabularInterval(const arma::mat& tab);
    arma::mat clippedTable() const;
};


#ifndef SWIG
std::unique_ptr<std::pair<time_t,boost::filesystem::path> >
newestOutputFile(
    const boost::filesystem::path& expectedFName
    );
#endif


std::map<std::string,arma::mat>
readAndCombineGroupedTabularFiles
(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& caseLocation,
    const std::string& FOName,
    const std::string& fileName,
    int groupByColumn,
    const std::string& filterChars="()",
    const std::string& regionName = std::string()
);



arma::mat
readAndCombineTabularFiles
(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& caseLocation,
    const std::string& FOName,
    const std::string& fileName,
    const std::string& filterChars="()",
    const std::string& regionName = std::string()
);

} // namespace insight

#endif // READPOSTPROCESSINGFILES_H
