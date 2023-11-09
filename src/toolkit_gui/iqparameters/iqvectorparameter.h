#ifndef IQVECTORPARAMETER_H
#define IQVECTORPARAMETER_H

#include "toolkit_gui_export.h"

#include <QLineEdit>
#include <iqparameter.h>

#include "base/parameters/simpleparameter.h"

class TOOLKIT_GUI_EXPORT IQVectorParameter : public IQParameter
{
public:
  declareType(insight::VectorParameter::typeName_());

    QLineEdit* lineEdit;

  IQVectorParameter
  (
      QObject* parent,
        IQParameterSetModel* psmodel,
      const QString& name,
      insight::Parameter& parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  QString valueText() const override;

  QVBoxLayout* populateEditControls(
          QWidget* editControlsContainer,
          IQCADModel3DViewer *viewer) override;

  void applyProposition(
      const insight::ParameterSet& propositions,
      const std::string& selectProposition ) override;

};

#endif // IQVECTORPARAMETER_H
