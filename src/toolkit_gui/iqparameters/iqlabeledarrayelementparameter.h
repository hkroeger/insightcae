#ifndef IQLABELEDARRAYELEMENTPARAMETER_H
#define IQLABELEDARRAYELEMENTPARAMETER_H


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
    declareType("IQLabeledArrayElementParameterBase");

    IQLabeledArrayElementParameterBase
        (
            QObject *parent,
            IQParameterSetModel* psmodel,
            insight::Parameter *parameter,
            const insight::ParameterSet &defaultParameterSet
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
            IQParameterSetModel* psmodel,
            insight::Parameter *parameter,
            const insight::ParameterSet &defaultParameterSet
            )
        : IQBaseParameter(parent, psmodel, parameter, defaultParameterSet),
        IQLabeledArrayElementParameterBase(parent, psmodel, parameter, defaultParameterSet)
    {}

    // const QString path(bool redirectArrayElementsToDefault=false) const override
    // {
    //     QString n=this->name();
    //     return this->buildPath(n, redirectArrayElementsToDefault);
    // }

    IQLabeledArrayParameter* arrayParameter()
    {
        return dynamic_cast<IQLabeledArrayParameter*>(this->parentParameter());
    }

    virtual void populateContextMenu(QMenu* m);

};


#endif // IQLABELEDARRAYELEMENTPARAMETER_H
