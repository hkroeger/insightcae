#include "casereader.h"

#include "openfoam/caseelements/evaluation/readpostprocessingfiles.h"
#include "openfoam/oftimedirectories.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <algorithm>
#include <set>
#include <fstream>
#include <iostream>

namespace fs = boost::filesystem;

const std::string selectionConfigFileName = "convergencebyvariance.cfg";


std::string Series::label() const
{
  return file+": "+column;
}


namespace {

bool isNumber(const std::string& s)
{
  try
  {
    size_t n;
    std::stod(s, &n);
    return n==s.size();
  }
  catch (...)
  {
    return false;
  }
}

std::vector<fs::path> sortedEntries(const fs::path& dir, bool directories)
{
  std::vector<fs::path> r;
  for (fs::directory_iterator i(dir); i!=fs::directory_iterator(); ++i)
  {
    if (directories ? fs::is_directory(i->status()) : fs::is_regular_file(i->status()))
      r.push_back(i->path());
  }
  std::sort(r.begin(), r.end());
  return r;
}

/**
 * strip restart suffix "_<number>" before extension:
 * force_37.96.dat => force.dat
 */
std::string baseFileName(const fs::path& f)
{
  static const boost::regex suffix("(.*)_[-+0-9.eE]+");
  std::string stem=f.stem().string();
  boost::smatch m;
  if (boost::regex_match(stem, m, suffix))
    stem=m[1];
  return stem+f.extension().string();
}

/**
 * convert table (first col: time) into series
 */
void appendSeries(
    CaseData& cd,
    const std::string& file,
    const arma::mat& data,
    const std::vector<std::string>& colNames )
{
  if (data.n_rows<1 || data.n_cols<2)
    throw std::runtime_error("no data columns");

  for (arma::uword j=1; j<data.n_cols; ++j)
  {
    std::string cn = j<colNames.size() ? colNames[j] : std::string();
    boost::erase_all(cn, "(");
    boost::erase_all(cn, ")");
    boost::trim(cn);
    if (cn.empty() || boost::starts_with(cn, "column"))
      cn="col "+std::to_string(j);

    cd.series.push_back({
        file, cn,
        arma::join_horiz(data.col(0), data.col(j)) });
  }
}

}



CaseData readCase(const fs::path& caseDir)
{
  CaseData cd;
  cd.name=caseDir.filename().string();

  // plain files
  for (const auto& f: sortedEntries(caseDir, false))
  {
    if (f.filename().string()==selectionConfigFileName)
      continue; // own config file (if data dir is the case dir itself)

    try
    {
      std::vector<std::string> colNames;
      auto d = insight::readSingleTabularFile(f, -1, "()", &colNames);
      if (d.empty())
        throw std::runtime_error("no data");
      appendSeries(cd, f.filename().string(), d.begin()->second, colNames);
    }
    catch (const std::exception& e)
    {
      cd.warnings.push_back(f.filename().string()+": "+e.what());
    }
  }

  // time directories: combine files of same name
  auto tdl = insight::listTimeDirectories(caseDir);
  std::set<std::string> fileNames;
  for (const auto& td: tdl)
  {
    for (const auto& f: sortedEntries(td.second, false))
      fileNames.insert(baseFileName(f));
  }
  for (const auto& fn: fileNames)
  {
    try
    {
      std::vector<std::string> colNames;
      auto d = insight::readAndCombineTabularFiles(caseDir, fn, "()", &colNames);
      appendSeries(cd, fn, d, colNames);
    }
    catch (const std::exception& e)
    {
      cd.warnings.push_back(fn+" (time directories): "+e.what());
    }
  }

  return cd;
}



std::vector<CaseData> readCases(const fs::path& dir)
{
  if (!fs::is_directory(dir))
    throw std::runtime_error("not a directory: "+dir.string());

  std::vector<fs::path> caseDirs;
  for (const auto& d: sortedEntries(dir, true))
  {
    if (!isNumber(d.filename().string()))
      caseDirs.push_back(d);
  }
  if (caseDirs.empty())
    caseDirs.push_back(dir);

  std::vector<CaseData> cases;
  for (const auto& cd: caseDirs)
    cases.push_back(readCase(cd));

  loadSelection(dir, cases);
  return cases;
}



fs::path selectionConfigFile(const fs::path& dataDir)
{
  return dataDir / selectionConfigFileName;
}



void loadSelection(const fs::path& dataDir, std::vector<CaseData>& cases)
{
  auto fn = selectionConfigFile(dataDir);
  if (!fs::exists(fn)) return;

  std::set<std::pair<std::string,std::string> > deselected;
  std::ifstream f(fn.string());
  std::string line;
  while (std::getline(f, line))
  {
    if (line.empty() || line[0]=='#') continue;
    auto i = line.find('\t');
    if (i==std::string::npos) continue;
    deselected.insert({ line.substr(0, i), line.substr(i+1) });
  }

  size_t nApplied=0;
  for (auto& cd: cases)
    for (auto& s: cd.series)
      if (deselected.count({cd.name, s.label()}))
      {
        s.selected=false;
        ++nApplied;
      }

  if (nApplied<deselected.size())
    std::cerr << "warning: " << deselected.size()-nApplied
              << " entries in " << fn.string()
              << " do not match any series" << std::endl;
}



void saveSelection(const fs::path& dataDir, const std::vector<CaseData>& cases)
{
  auto fn = selectionConfigFile(dataDir);

  std::vector<std::string> lines;
  std::set<std::string> known;
  for (const auto& cd: cases)
    for (const auto& s: cd.series)
    {
      std::string l = cd.name+"\t"+s.label();
      known.insert(l);
      if (!s.selected)
        lines.push_back(l);
    }

  // keep entries of series, which are currently not present
  // (e.g. file temporarily missing or unreadable)
  if (fs::exists(fn))
  {
    std::ifstream f(fn.string());
    std::string l;
    while (std::getline(f, l))
      if (!l.empty() && l[0]!='#' && l.find('\t')!=std::string::npos && !known.count(l))
        lines.push_back(l);
  }

  if (lines.empty())
  {
    if (fs::exists(fn)) fs::remove(fn);
    return;
  }

  std::ofstream f(fn.string());
  if (!f)
    throw std::runtime_error("could not write "+fn.string());
  f << "# deselected series of test_convergencebyvariance" << std::endl
    << "# <case>\t<file>: <column>" << std::endl;
  for (const auto& l: lines)
    f << l << std::endl;
}
