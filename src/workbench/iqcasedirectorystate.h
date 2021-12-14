#ifndef IQCASEDIRECTORYSTATE_H
#define IQCASEDIRECTORYSTATE_H

#include <QString>

#include "base/casedirectory.h"

class AnalysisForm;

/**
 * @brief The IQCaseDirectory class
 * sets GUI enable/disable states, inserts text
 */
class IQCaseDirectoryState
    : public insight::CaseDirectory
{
  AnalysisForm *af_;
  void updateGUI(bool enabled);

public:
  template<class ...Args>
  IQCaseDirectoryState(AnalysisForm *af, Args&&... addArgs)
      : insight::CaseDirectory( std::forward<Args>(addArgs)... ),
        af_(af)
  {
    updateGUI(true);
  }

  ~IQCaseDirectoryState();

  operator QString() const;
};

#endif // IQCASEDIRECTORYSTATE_H
