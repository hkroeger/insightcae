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

#ifndef SWIG
/**
 * Like above, but reads directly from a directory, which contains
 * the time directories (e.g. postProcessing/<FOName>).
 * In each time directory, the newest file matching
 * fileName (optionally with suffix "_<anything>" before the extension) is read.
 * The time segments are combined in the order of the file modification times,
 * overlapping parts are taken from the newer files.
 */
std::map<std::string,arma::mat>
readAndCombineGroupedTabularFiles
(
    const boost::filesystem::path& dataDir,
    const std::string& fileName,
    int groupByColumn,
    const std::string& filterChars="()",
    std::vector<std::string>* columnNames = nullptr
);

arma::mat
readAndCombineTabularFiles
(
    const boost::filesystem::path& dataDir,
    const std::string& fileName,
    const std::string& filterChars="()",
    std::vector<std::string>* columnNames = nullptr
);
#endif

} // namespace insight

#endif // READPOSTPROCESSINGFILES_H
