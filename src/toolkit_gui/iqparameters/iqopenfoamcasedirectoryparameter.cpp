#include "iqopenfoamcasedirectoryparameter.h"

defineType(IQOpenFOAMCaseDirectoryParameter);
addToFactoryTable(IQParameter, IQOpenFOAMCaseDirectoryParameter);

IQOpenFOAMCaseDirectoryParameter::IQOpenFOAMCaseDirectoryParameter
(
    QObject* parent,
    IQHierarchicalDataModel* hdmodel,
    insight::hierarchicalData::Element* element
)
  : IQDirectoryParameter(parent, hdmodel, element)
{
}
