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
            IQHierarchicalDataModel* hdmodel,
            insight::hierarchicalData::Element* element
            );

    QVariant value() const override;


    QVBoxLayout* populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer ) override;

    void populateContextMenu(QMenu* m) override;

    IQHierarchicalDataElement *createForChild(
        IQHierarchicalDataModel *model,
        insight::hierarchicalData::Element *ce ) override;

private Q_SLOTS:
    void appendEmpty();
    void clearAll();
};

#endif // IQARRAYPARAMETER_H
