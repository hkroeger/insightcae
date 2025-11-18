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
            IQHierarchicalDataModel* hdmodel,
            insight::hierarchicalData::Element* arrayelement ),
        LIST(parent, hdmodel, arrayelement)
  );

public:
  declareType("IQArrayElementParameterBase");

  IQArrayElementParameterBase
  (
      QObject *parent,
      IQHierarchicalDataModel* hdmodel,
      insight::hierarchicalData::Element* element
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
      IQHierarchicalDataModel* hdmodel,
      insight::hierarchicalData::Element* element
  )
    : IQBaseParameter(parent, hdmodel, element),
      IQArrayElementParameterBase(parent, hdmodel, element)
  {}

  void populateContextMenu(QMenu* m, IQCADModel3DViewer *viewer) override;

  void deleteFromArray() override;
};

#endif // IQARRAYELEMENTPARAMETER_H
