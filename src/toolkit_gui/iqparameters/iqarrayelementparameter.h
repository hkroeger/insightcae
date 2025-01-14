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
            insight::Parameter* parameter,
            const insight::ParameterSet& defaultParameterSet ),
        LIST(parent, psmodel, parameter, defaultParameterSet)
  );

  static IQParameter* create(
      QObject* parent,
      IQParameterSetModel* psmodel,
      insight::Parameter* p,
      const insight::ParameterSet& defaultParameterSet );

public:
  declareType("IQArrayElementParameterBase");

  IQArrayElementParameterBase
  (
      QObject *parent,
      IQParameterSetModel* psmodel,
      insight::Parameter *parameter,
      const insight::ParameterSet &defaultParameterSet
  );

public Q_SLOTS:
  virtual void deleteFromArray() =0;
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
      insight::Parameter *parameter,
      const insight::ParameterSet &defaultParameterSet
  )
    : IQBaseParameter(parent, psmodel, parameter, defaultParameterSet),
      IQArrayElementParameterBase(parent, psmodel, parameter, defaultParameterSet)
  {}

  void populateContextMenu(QMenu* m) override;

  void deleteFromArray() override;
};

#endif // IQARRAYELEMENTPARAMETER_H
