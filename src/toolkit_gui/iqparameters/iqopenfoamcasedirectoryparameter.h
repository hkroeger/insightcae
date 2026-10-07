#ifndef IQOPENFOAMCASEDIRECTORYPARAMETER_H
#define IQOPENFOAMCASEDIRECTORYPARAMETER_H

#include "toolkit_gui_export.h"

#include "iqdirectoryparameter.h"

#include "openfoam/openfoamcasedirectoryparameter.h"

class TOOLKIT_GUI_EXPORT IQOpenFOAMCaseDirectoryParameter
    : public IQDirectoryParameter
{
public:
  declareType(insight::OpenFOAMCaseDirectoryParameter::typeName_());

  IQOpenFOAMCaseDirectoryParameter
  (
      QObject* parent,
      IQHierarchicalDataModel* hdmodel,
      insight::hierarchicalData::Element* element
  );

};

#endif // IQOPENFOAMCASEDIRECTORYPARAMETER_H
