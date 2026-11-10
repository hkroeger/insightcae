#ifndef CONVERGENCEBYVARIANCE_CASEREADER_H
#define CONVERGENCEBYVARIANCE_CASEREADER_H

#include "base/linearalgebra.h"

#include <boost/filesystem/path.hpp>
#include <string>
#include <vector>

/**
 * one data column of one file: time history
 */
struct Series
{
  std::string file, column;
  arma::mat tx; // n x 2: time, value
  bool selected = true; // deselected series are ignored in the case status

  std::string label() const;
};

struct CaseData
{
  std::string name;
  std::vector<Series> series;
  std::vector<std::string> warnings; // files which could not be read
};

/**
 * Reads all files of one case directory:
 *  - plain files are read individually
 *  - files in numeric subdirectories (time directories, e.g. from restarts)
 *    are combined by name using insight::readAndCombineTabularFiles
 * The first column of each file is time, all other columns become series.
 */
CaseData readCase(const boost::filesystem::path& caseDir);

/**
 * Each subdirectory of dir is a case.
 * If there are no (non-numeric) subdirectories, dir itself is the only case.
 */
std::vector<CaseData> readCases(const boost::filesystem::path& dir);

/**
 * name of the file in the data directory, which stores the deselected series
 * (one line per series: <case name><TAB><series label>)
 */
extern const std::string selectionConfigFileName;

boost::filesystem::path selectionConfigFile(const boost::filesystem::path& dataDir);

/**
 * apply stored selection (called by readCases)
 */
void loadSelection(const boost::filesystem::path& dataDir, std::vector<CaseData>& cases);

/**
 * store deselected series. File is removed, if all series are selected.
 */
void saveSelection(const boost::filesystem::path& dataDir, const std::vector<CaseData>& cases);

#endif
