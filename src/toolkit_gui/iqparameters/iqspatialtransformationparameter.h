#ifndef IQSPATIALTRANSFORMATIONPARAMETER_H
#define IQSPATIALTRANSFORMATIONPARAMETER_H

#include "toolkit_gui_export.h"

#include <iqparameter.h>
#include "base/parameters/spatialtransformationparameter.h"


class TOOLKIT_GUI_EXPORT IQSpatialTransformationParameter
    : public IQSpecializedParameter<insight::SpatialTransformationParameter>
{
public:
    declareType(insight::SpatialTransformationParameter::typeName_());

    IQSpatialTransformationParameter
    (
        QObject* parent,
        IQParameterSetModel* psmodel,
        insight::Parameter* parameter,
        const insight::ParameterSet& defaultParameterSet
    );

    QString valueText() const override;

    QVBoxLayout* populateEditControls(
            QWidget* editControlsContainer,
            IQCADModel3DViewer *viewer ) override;

};

#endif // IQSPATIALTRANSFORMATIONPARAMETER_H
