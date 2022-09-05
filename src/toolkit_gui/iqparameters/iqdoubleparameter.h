#ifndef IQDOUBLEPARAMETER_H
#define IQDOUBLEPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>
#include "base/parameters/simpleparameter.h"

class TOOLKIT_GUI_EXPORT IQDoubleParameter : public IQParameter
{
public:
    declareType(insight::DoubleParameter::typeName_());

    IQDoubleParameter
    (
        QObject* parent,
        const QString& name,
        insight::Parameter& parameter,
        const insight::ParameterSet& defaultParameterSet
    );

    QString valueText() const override;

    QVBoxLayout* populateEditControls(
            IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer,
            IQCADModel3DViewer *viewer ) override;

};

#endif // IQDOUBLEPARAMETER_H
