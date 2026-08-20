#ifndef READPOSTPROCESSINGFILES_H
#define READPOSTPROCESSINGFILES_H

#include "openfoam/openfoamcase.h"
#include "base/intervals.h"

#include <string>
#include <vector>
#include <map>


namespace insight {



class TabularInterval
    : public Interval
{
    arma::mat table_;
    std::vector<std::string> colNames_;
public:
    TabularInterval(const arma::mat& tab,
                    std::vector<std::string> colNames);
    arma::mat clippedTable() const;
    const std::vector<std::string>& colNames() const;
};


#ifndef SWIG
std::unique_ptr<std::pair<time_t,boost::filesystem::path> >
newestOutputFile(
    const boost::filesystem::path& expectedFName
    );

std::map<std::string, arma::mat>
readSingleTabularFile(
    const boost::filesystem::path& ffp,
    int groupByColumn,
    const std::string& filterChars,
    std::vector<std::string>* columnNames = nullptr
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
    const std::string& regionName = std::string(),
    std::vector<std::string>* columnNames = nullptr
);



arma::mat
readAndCombineTabularFiles
(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& caseLocation,
    const std::string& FOName,
    const std::string& fileName,
    const std::string& filterChars="()",
    const std::string& regionName = std::string(),
    std::vector<std::string>* columnNames = nullptr
);

} // namespace insight

#endif // READPOSTPROCESSINGFILES_H
