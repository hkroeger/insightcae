#ifndef IQSELECTABLESUBSETPARAMETER_H
#define IQSELECTABLESUBSETPARAMETER_H

#include "toolkit_gui_export.h"

#include <QStyledItemDelegate>

#include "iqselectionparameter.h"
#include "base/parameters/selectablesubsetparameter.h"

class TOOLKIT_GUI_EXPORT IQSelectableSubsetParameter
    : public IQSelectionParameterBase<insight::SelectableSubsetParameter>
{
public:
  declareType(insight::SelectableSubsetParameter::typeName_());

  IQSelectableSubsetParameter
  (
      QObject* parent,
      IQHierarchicalDataModel* hdmodel,
      insight::hierarchicalData::Element* element
  );

  void populateContextMenu(QMenu* m) override;

  QVariant value() const override;
};




#endif // IQSELECTABLESUBSETPARAMETER_H
