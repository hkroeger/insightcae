#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>

#include "iqlabeledarraykeyselectionparameter.h"
#include "iqparametersetmodel.h"
#include "qtextensions.h"


defineTemplateType(IQLabeledArrayKeySelectionParameterBase);
defineType(IQLabeledArrayKeySelectionParameter);
addToFactoryTable(IQParameter, IQLabeledArrayKeySelectionParameter);

addFunctionToStaticFunctionTable(
    IQHierarchicalDataGridViewDelegateEditorWidget,
    IQLabeledArrayKeySelectionParameter,
    createDelegate,
    [](QObject* parent) { return new IQSelectionDelegate(parent); }
    );


IQLabeledArrayKeySelectionParameter::IQLabeledArrayKeySelectionParameter(
    QObject* parent,
    IQHierarchicalDataModel* hdmodel,
    insight::hierarchicalData::Element* element)
    : IQLabeledArrayKeySelectionParameterBase(
          parent, hdmodel, element)
{}


QVBoxLayout* IQLabeledArrayKeySelectionParameter::populateEditControls(
    QWidget* editControlsContainer, IQCADModel3DViewer* viewer)
{
    // Call the grandparent directly so we own the combo box variable and can
    // capture it in the repopulate lambda below.
    auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

    QHBoxLayout* layout2 = new QHBoxLayout;
    QLabel* promptLabel = new QLabel("Selection:", editControlsContainer);
    layout2->addWidget(promptLabel);

    auto* selBox = new QComboBox(editControlsContainer);
    selBox->setIconSize(QSize(200, 150));

    // Helper to (re-)populate the combo box from the current key set.
    // When bound: non-editable combo restricted to array keys.
    // When unbound: editable combo acting as a free-text field.
    auto repopulate = [this, selBox]()
    {
        QSignalBlocker sb(selBox);
        selBox->clear();
        const bool bound = this->parameter().isBound();
        selBox->setEditable(!bound);
        if (bound)
        {
            for (const auto& k: this->parameter().selectionKeys())
            {
                auto qk = QString::fromStdString(k);
                auto ip = this->parameter().iconPathForKey(k);
                if (ip.empty())
                    selBox->addItem(qk);
                else
                    selBox->addItem(QPixmap(QString::fromStdString(ip)), qk);
            }
            auto idx = this->parameter().selectionIndex();
            selBox->setCurrentIndex(idx >= 0 ? idx : 0);
        }
        else
        {
            selBox->setCurrentText(
                QString::fromStdString(this->parameter().selection()));
        }
    };

    repopulate();

    layout2->addWidget(selBox);
    layout->addLayout(layout2);

    QPushButton* apply = new QPushButton("&Apply", editControlsContainer);
    QObject::connect(apply, &QPushButton::pressed, [this, selBox]()
    {
        if (this->parameter().isBound())
            this->parameterRef().setSelectionFromIndex(selBox->currentIndex());
        else
            this->parameterRef().set(selBox->currentText().toStdString());
    });
    layout->addWidget(apply);

    // Refresh the combo box whenever the referenced array's key set changes.
    // disconnectAtEOL ensures the boost connection is cleaned up when the
    // edit panel widget (layout) is destroyed.
    ::disconnectAtEOL(
        layout,
        parameterRef().valueChanged.connect(repopulate)
    );

    return layout;
}
