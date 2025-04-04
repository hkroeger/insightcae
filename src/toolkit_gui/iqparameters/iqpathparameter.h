#ifndef IQPATHPARAMETER_H
#define IQPATHPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/pathparameter.h"

class TOOLKIT_GUI_EXPORT IQPathParameter
    : public IQSpecializedParameter<insight::PathParameter>
{
public:
  declareType(insight::PathParameter::typeName_());

  IQPathParameter
  (
      QObject* parent,
      IQParameterSetModel* psmodel,
      insight::Parameter* parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  QString valueText() const override;

  QVBoxLayout* populateEditControls(
          QWidget* editControlsContainer,
          IQCADModel3DViewer *viewer) override;

  virtual QString showSelectPathDialog(QWidget* parent, const QString& startPath) const;

};


#endif // IQPATHPARAMETER_H
