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
      IQParameterSetModel* psmodel,
      insight::Parameter* parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  void populateContextMenu(QMenu* m) override;
};




#endif // IQSELECTABLESUBSETPARAMETER_H
