#ifndef IQINTPARAMETER_H
#define IQINTPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/simpleparameter.h"


class TOOLKIT_GUI_EXPORT IQIntParameter
    : public IQSpecializedParameter<insight::IntParameter>
{
public:
    declareType(insight::IntParameter::typeName_());

    IQIntParameter
    (
        QObject* parent,
        IQHierarchicalDataModel* hdmodel,
        insight::hierarchicalData::Element* element
    );


    QVBoxLayout* populateEditControls(
            QWidget* editControlsContainer,
            IQCADModel3DViewer *viewer) override;
};

#endif // IQINTPARAMETER_H
