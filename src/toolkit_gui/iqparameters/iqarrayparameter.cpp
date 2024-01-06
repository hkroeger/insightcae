#include <QHBoxLayout>
#include <QPushButton>


#include "iqarrayparameter.h"
#include "iqparametersetmodel.h"

defineType(IQArrayParameter);
addToFactoryTable(IQParameter, IQArrayParameter);

IQArrayParameter::IQArrayParameter
(
    QObject* parent,
    IQParameterSetModel* psmodel,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
) : IQParameter(parent, psmodel, name, parameter, defaultParameterSet)
{}


QString IQArrayParameter::valueText() const
{
  auto& p = dynamic_cast<const insight::ArrayParameter&>(parameter());
  return QString( "array[%1]" ).arg(p.size());
}




QVBoxLayout* IQArrayParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;

  QPushButton *addbtn=new QPushButton("+ Add new", editControlsContainer);
  layout2->addWidget(addbtn);

  connect(addbtn, &QPushButton::clicked, this, [this]()
  {
    auto &p = dynamic_cast<insight::ArrayParameter&>(this->parameterRef());
    p.appendEmpty();
  }
  );

  QPushButton *clearbtn=new QPushButton("Clear all", editControlsContainer);
  layout2->addWidget(clearbtn);
  connect(clearbtn, &QPushButton::clicked, this, [this]()
  {

    auto &p = dynamic_cast<insight::ArrayParameter&>(this->parameterRef());
    p.clear();
  }
  );

  layout->addLayout(layout2);

  return layout;
}
