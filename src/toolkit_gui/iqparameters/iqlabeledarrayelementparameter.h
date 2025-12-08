#ifndef IQLABELEDARRAYELEMENTPARAMETER_H
#define IQLABELEDARRAYELEMENTPARAMETER_H


#include "base/hierarchicalelement.h"
#include "toolkit_gui_export.h"


#include <iqparameter.h>

class IQLabeledArrayParameter;


class TOOLKIT_GUI_EXPORT IQLabeledArrayElementParameterBase
{
public:
    declareFactoryTable
        (
            IQLabeledArrayElementParameterBase,
            LIST(
                QObject* parent,
                IQHierarchicalDataModel* psmodel,
                insight::hierarchicalData::Element* parameter ),
            LIST(parent, psmodel, parameter)
            );

public:
    declareType("IQLabeledArrayElementParameterBase");

    IQLabeledArrayElementParameterBase
        (
            QObject *parent,
            IQHierarchicalDataModel* psmodel,
            insight::hierarchicalData::Element *parameter
            );
};




template<class IQBaseParameter, const char* N>
class IQLabeledArrayElementParameter
    : public IQBaseParameter,
      public IQLabeledArrayElementParameterBase
{


public:
    declareType(N);

    IQLabeledArrayElementParameter
        (
            QObject *parent,
            IQHierarchicalDataModel* psmodel,
            insight::hierarchicalData::Element *parameter
            )
        : IQBaseParameter(parent, psmodel, parameter),
        IQLabeledArrayElementParameterBase(parent, psmodel, parameter)
    {}


    IQLabeledArrayParameter* arrayParameter()
    {
        return dynamic_cast<IQLabeledArrayParameter*>(this->parentElement());
    }

    virtual void populateContextMenu(QMenu* m, IQCADModel3DViewer *viewer);
};


#endif // IQLABELEDARRAYELEMENTPARAMETER_H
