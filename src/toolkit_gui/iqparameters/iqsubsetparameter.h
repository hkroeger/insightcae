#ifndef IQSUBSETPARAMETER_H
#define IQSUBSETPARAMETER_H

#include "toolkit_gui_export.h"
#include <iqparameter.h>
#include "base/parameters/subsetparameter.h"

class TOOLKIT_GUI_EXPORT IQSubsetParameter
    : public IQSpecializedParameter<insight::ParameterSet>
{
public:
    declareType(insight::ParameterSet::typeName_());

    IQSubsetParameter
    (
        QObject* parent,
        IQParameterSetModel* psmodel,
        insight::Parameter* parameter,
        const insight::ParameterSet& defaultParameterSet
    );

    void populateContextMenu(QMenu* m) override;
};

#endif // IQSUBSETPARAMETER_H
