#ifndef IQARRAYPARAMETER_H
#define IQARRAYPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/arrayparameter.h"

class TOOLKIT_GUI_EXPORT IQArrayParameter
    : public IQSpecializedParameter<insight::ArrayParameter>
{
    Q_OBJECT

public:
    declareType(insight::ArrayParameter::typeName_());

    IQArrayParameter
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

    void populateContextMenu(QMenu* m) override;

private Q_SLOTS:
    void appendEmpty();
    void clearAll();
};

#endif // IQARRAYPARAMETER_H
