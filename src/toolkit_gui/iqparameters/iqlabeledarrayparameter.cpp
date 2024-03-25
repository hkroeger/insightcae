#include <QHBoxLayout>
#include <QPushButton>

#include "iqlabeledarrayparameter.h"

defineType(IQLabeledArrayParameter);
addToFactoryTable(IQParameter, IQLabeledArrayParameter);

IQLabeledArrayParameter::IQLabeledArrayParameter
    (
        QObject* parent,
        IQParameterSetModel* psmodel,
        const QString& name,
        insight::Parameter& parameter,
        const insight::ParameterSet& defaultParameterSet
        ) : IQParameter(parent, psmodel, name, parameter, defaultParameterSet)
{}


QString IQLabeledArrayParameter::valueText() const
{
    auto& p = dynamic_cast<const insight::LabeledArrayParameter&>(parameter());
    return QString( "labeledarray[%1]" ).arg(p.size());
}




QVBoxLayout* IQLabeledArrayParameter::populateEditControls(
    QWidget* editControlsContainer,
    IQCADModel3DViewer *viewer)
{
    auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

    QHBoxLayout *layout2=new QHBoxLayout;

    QPushButton *addbtn=new QPushButton("+ Add new", editControlsContainer);
    layout2->addWidget(addbtn);

    connect(addbtn, &QPushButton::clicked, this, [this]()
            {
                auto &p = dynamic_cast<insight::LabeledArrayParameter&>(this->parameterRef());
                p.appendEmpty();
            }
            );

    QPushButton *clearbtn=new QPushButton("Clear all", editControlsContainer);
    layout2->addWidget(clearbtn);
    connect(clearbtn, &QPushButton::clicked, this, [this]()
            {

                auto &p = dynamic_cast<insight::LabeledArrayParameter&>(this->parameterRef());
                p.clear();
            }
            );

    layout->addLayout(layout2);

    return layout;
}
