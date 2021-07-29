#ifndef IQCASEDIRECTORYSTATE_H
#define IQCASEDIRECTORYSTATE_H

#include <QString>

#include "base/tools.h"

class AnalysisForm;

/**
 * @brief The IQCaseDirectory class
 * sets GUI enable/disable states, inserts text
 */
class IQCaseDirectoryState
    : public insight::CaseDirectory
{
  AnalysisForm *af_;

  void setAFEnabledState(bool enabled);

public:
  IQCaseDirectoryState(AnalysisForm *af, const boost::filesystem::path& path, bool keep=true);
  IQCaseDirectoryState(AnalysisForm *af, bool keep=true, const boost::filesystem::path& prefix="");
  ~IQCaseDirectoryState();

  operator QString() const;
};

#endif // IQCASEDIRECTORYSTATE_H
