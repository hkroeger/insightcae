#ifndef IQDATETIMEPARAMETER_H
#define IQDATETIMEPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/simpleparameter.h"

class TOOLKIT_GUI_EXPORT IQDateTimeParameter
    : public IQSpecializedParameter<insight::DateTimeParameter>
{
public:
    declareType(insight::DateTimeParameter::typeName_());

    IQDateTimeParameter
        (
            QObject* parent,
            IQHierarchicalDataModel* hdmodel,
            insight::hierarchicalData::Element* element
            );


    QVBoxLayout* populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer) override;
};

#endif // IQDATETIMEPARAMETER_H
