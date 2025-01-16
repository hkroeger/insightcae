#include <QHBoxLayout>
#include <QPushButton>


#include "iqarrayelementparameter.h"
#include "iqarrayparameter.h"
#include "iqparametersetmodel.h"

defineType(IQArrayParameter);
addToFactoryTable(IQParameter, IQArrayParameter);

IQArrayParameter::IQArrayParameter
(
    QObject* parent,
    IQParameterSetModel* psmodel,
    insight::Parameter* parameter,
    const insight::ParameterSet& defaultParameterSet
)
    : IQSpecializedParameter<insight::ArrayParameter>(
          parent, psmodel, parameter, defaultParameterSet)
{}


QString IQArrayParameter::valueText() const
{
  return QString( "array[%1]" )
        .arg( parameter().size() );
}




QVBoxLayout* IQArrayParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;

  QPushButton *addbtn=new QPushButton("+ Add new", editControlsContainer);
  layout2->addWidget(addbtn);

  connect(addbtn, &QPushButton::clicked,
          this, &IQArrayParameter::appendEmpty );

  QPushButton *clearbtn=new QPushButton("Clear all", editControlsContainer);
  layout2->addWidget(clearbtn);
  connect(clearbtn, &QPushButton::clicked,
          this, &IQArrayParameter::clearAll );

  layout->addLayout(layout2);

  return layout;
}




void IQArrayParameter::populateContextMenu(QMenu *cm)
{
    auto *ca = new QAction("Clear array");
    cm->addAction(ca);
    connect(ca, &QAction::triggered,
            this, &IQArrayParameter::clearAll );

    auto *ap = new QAction("Append new element");
    cm->addAction(ap);
    connect(ap, &QAction::triggered,
            this, &IQArrayParameter::appendEmpty );
}




void IQArrayParameter::appendEmpty()
{
    parameterRef().appendEmpty(true);
}




void IQArrayParameter::clearAll()
{
    while (children().size())
    {
        auto c=children().front();
        if (auto *ae = dynamic_cast<IQArrayElementParameterBase*>(c))
            ae->deleteFromArray();
    }
}
