#ifndef IQARRAYELEMENTPARAMETER_H
#define IQARRAYELEMENTPARAMETER_H

#include <iqparameter.h>

class IQArrayElementParameterBase
{
public:
  declareFactoryTable
  (
      IQArrayElementParameterBase,
        LIST(QObject* parent, const QString& name, insight::Parameter& parameter, const insight::ParameterSet& defaultParameterSet),
        LIST(parent, name, parameter, defaultParameterSet)
  );

  static IQParameter* create(QObject* parent, const QString& name, insight::Parameter& p, const insight::ParameterSet& defaultParameterSet);

public:
  declareType("IQArrayElementParameterBase");

  IQArrayElementParameterBase
  (
      QObject *parent,
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
      const QString &name,
      insight::Parameter &parameter,
      const insight::ParameterSet &defaultParameterSet
  )
    : IQBaseParameter(parent, name, parameter, defaultParameterSet),
      IQArrayElementParameterBase(parent, name, parameter, defaultParameterSet)
  {}

  const QString path(bool redirectArrayElementsToDefault=false) const override
  {
    QString n=this->name();
    if (redirectArrayElementsToDefault) n="default";
    return this->buildPath(n, redirectArrayElementsToDefault);
  }


  virtual void populateContextMenu(IQParameterSetModel* model, const QModelIndex &index, QMenu* m);
//  virtual QVBoxLayout* populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer);
};

#endif // IQARRAYELEMENTPARAMETER_H
