#ifndef IQDOUBLEPARAMETER_H
#define IQDOUBLEPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>
#include "base/parameters/simpleparameter.h"

class TOOLKIT_GUI_EXPORT IQDoubleParameter
    : public IQSpecializedParameter<insight::DoubleParameter>
{
public:
    declareType(insight::DoubleParameter::typeName_());

    IQDoubleParameter
    (
        QObject* parent,
        IQHierarchicalDataModel* hdmodel,
        insight::hierarchicalData::Element* element
    );

    QVBoxLayout* populateEditControls(
            QWidget* editControlsContainer,
            IQCADModel3DViewer *viewer ) override;

};

#endif // IQDOUBLEPARAMETER_H
