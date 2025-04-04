#include <QHBoxLayout>
#include <QPushButton>

#include "iqlabeledarrayparameter.h"

defineType(IQLabeledArrayParameter);
addToFactoryTable(IQParameter, IQLabeledArrayParameter);

IQLabeledArrayParameter::IQLabeledArrayParameter
    (
        QObject* parent,
        IQParameterSetModel* psmodel,
        insight::Parameter* parameter,
        const insight::ParameterSet& defaultParameterSet
        ) : IQSpecializedParameter<insight::LabeledArrayParameter>(
          parent, psmodel, parameter, defaultParameterSet)
{}


QString IQLabeledArrayParameter::valueText() const
{
    return QString( "labeledarray[%1]" )
        .arg(parameter().size());
}




QVBoxLayout* IQLabeledArrayParameter::populateEditControls(
    QWidget* editControlsContainer,
    IQCADModel3DViewer *viewer)
{
    auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

    QHBoxLayout *layout2=new QHBoxLayout;

    if (!parameter().keysAreLocked())
    {
        QPushButton *addbtn=new QPushButton("+ Add new", editControlsContainer);
        layout2->addWidget(addbtn);

        connect(addbtn, &QPushButton::clicked, this, [this]()
                {
                    parameterRef().appendEmpty();
                }
                );

        QPushButton *clearbtn=new QPushButton("Clear all", editControlsContainer);
        layout2->addWidget(clearbtn);
        connect(clearbtn, &QPushButton::clicked, this, [this]()
                {
                    auto &p=parameterRef();
                    auto keys=p.keys();
                    while (keys.size())
                    {
                        auto k=*keys.begin();
                        p.eraseValue(k);
                        keys.erase(k);
                    }
                }
                );
    }

    layout->addLayout(layout2);

    return layout;
}
