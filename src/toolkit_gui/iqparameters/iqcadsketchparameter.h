#ifndef IQCADSKETCHPARAMETER_H
#define IQCADSKETCHPARAMETER_H


#include "iqparameter.h"
#include "cadsketchparameter.h"


class IQCADSketchParameter : public IQParameter
{
public:
    declareType(insight::CADSketchParameter::typeName_());

    IQCADSketchParameter
        (
            QObject* parent,
        IQParameterSetModel* psmodel,
            const QString& name,
            insight::Parameter& parameter,
            const insight::ParameterSet& defaultParameterSet
            );

    void connectSignals() override;

    QString valueText() const override;

    QVBoxLayout* populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer) override;
};



#endif // IQCADSKETCHPARAMETER_H
