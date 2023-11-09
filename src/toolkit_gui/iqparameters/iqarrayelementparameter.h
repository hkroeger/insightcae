#ifndef IQARRAYELEMENTPARAMETER_H
#define IQARRAYELEMENTPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

class TOOLKIT_GUI_EXPORT IQArrayElementParameterBase
{
public:
  declareFactoryTable
  (
      IQArrayElementParameterBase,
        LIST(
            QObject* parent,
            IQParameterSetModel* psmodel,
            const QString& name,
            insight::Parameter& parameter,
            const insight::ParameterSet& defaultParameterSet ),
        LIST(parent, psmodel, name, parameter, defaultParameterSet)
  );

  static IQParameter* create(
      QObject* parent,
      IQParameterSetModel* psmodel,
      const QString& name,
      insight::Parameter& p,
      const insight::ParameterSet& defaultParameterSet );

public:
  declareType("IQArrayElementParameterBase");

  IQArrayElementParameterBase
  (
      QObject *parent,
      IQParameterSetModel* psmodel,
      const QString &name,
      insight::Parameter &parameter,
      const insight::ParameterSet &defaultParameterSet
  );
};




template<class IQBaseParameter, const char* N>
class IQArrayElementParameter
    : public IQBaseParameter,
      public IQArrayElementParameterBase
{


public:
  declareType(N);

  IQArrayElementParameter
  (
      QObject *parent,
      IQParameterSetModel* psmodel,
      const QString &name,
      insight::Parameter &parameter,
      const insight::ParameterSet &defaultParameterSet
  )
    : IQBaseParameter(parent, psmodel, name, parameter, defaultParameterSet),
      IQArrayElementParameterBase(parent, psmodel, name, parameter, defaultParameterSet)
  {}

  const QString path(bool redirectArrayElementsToDefault=false) const override
  {
    QString n=this->name();
    if (redirectArrayElementsToDefault) n="default";
    return this->buildPath(n, redirectArrayElementsToDefault);
  }


  virtual void populateContextMenu(QMenu* m);
//  virtual QVBoxLayout* populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer);
};

#endif // IQARRAYELEMENTPARAMETER_H
