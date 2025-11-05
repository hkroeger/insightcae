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
        IQHierarchicalDataModel* hdmodel,
        insight::hierarchicalData::Element* element
    );

    void populateContextMenu(QMenu* m, IQCADModel3DViewer *viewer) override;

    QVariant value() const override;
};

#endif // IQSUBSETPARAMETER_H
