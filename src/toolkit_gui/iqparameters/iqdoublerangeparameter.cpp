#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QInputDialog>

#include "iqdoublerangeparameter.h"
#include "iqparametersetmodel.h"


defineType(IQDoubleRangeParameter);
addToFactoryTable(IQParameter, IQDoubleRangeParameter);

IQDoubleRangeParameter::IQDoubleRangeParameter
(
    QObject* parent,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, name, parameter, defaultParameterSet)
{
}


QString IQDoubleRangeParameter::valueText() const
{
  const auto& p =dynamic_cast<const insight::DoubleRangeParameter&>(parameter());
  return QString("%1 values").arg( p.values().size() );
}



QVBoxLayout* IQDoubleRangeParameter::populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer)
{
  const auto& p =dynamic_cast<const insight::DoubleRangeParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer);

  QLabel *promptLabel = new QLabel("Selection:", editControlsContainer);
  layout->addWidget(promptLabel);

  QHBoxLayout *layout2=new QHBoxLayout(editControlsContainer);
  auto *listBox = new QListWidget(editControlsContainer);
//  connect(lBox_, &QListWidget::destroyed, this, &DoubleRangeParameterWrapper::onDestruction);
  layout2->addWidget(listBox);

  auto rebuildList = [=](const insight::DoubleRangeParameter& cp)
  {
    int crow=listBox->currentRow();
    listBox->clear();
    for (insight::DoubleRangeParameter::RangeList::const_iterator i=cp.values().begin(); i!=cp.values().end(); i++)
    {
      listBox->addItem( QString::number(*i) );
    }
    listBox->setCurrentRow(crow);
  };
  rebuildList(p);

  QVBoxLayout *sublayout=new QVBoxLayout(editControlsContainer);

  QPushButton *addbtn=new QPushButton("Add...", editControlsContainer);
  sublayout->addWidget(addbtn);
  connect(addbtn, &QPushButton::clicked, [=]()
  {
    bool ok;
    double v=QInputDialog::getDouble(editControlsContainer, "Add Range", "Please specify value:", 0., -2147483647,  2147483647, 9, &ok);
    if (ok)
    {
      auto&p = dynamic_cast<insight::DoubleRangeParameter&>(model->parameterRef(index));
      p.insertValue(v);
      rebuildList(p);
      model->notifyParameterChange(index);
    }
  }
  );

  QPushButton *addrangebtn=new QPushButton("Add Range...", editControlsContainer);
  sublayout->addWidget(addrangebtn);
  connect(addrangebtn, &QPushButton::clicked, [=]()
  {
    QString res=QInputDialog::getText(editControlsContainer, "Add Range", "Please specify range begin, range end and number of values, separated by spaces:");
    if (!res.isEmpty())
    {
      auto&p = dynamic_cast<insight::DoubleRangeParameter&>(model->parameterRef(index));

      QStringList il=res.split(" ", Qt::SkipEmptyParts);
      double x0=il[0].toDouble();
      double x1=il[1].toDouble();
      int num=il[2].toInt();
      for (int i=0; i<num; i++)
      {
        double x=x0+(x1-x0)*double(i)/double(num-1);
        p.insertValue(x);
        rebuildList(p);
        model->notifyParameterChange(index);
      }
    }
  }
  );

  QPushButton *clearbtn=new QPushButton("Clear", editControlsContainer);
  sublayout->addWidget(clearbtn);
  connect(clearbtn, &QPushButton::clicked, [=]()
  {
    auto&p = dynamic_cast<insight::DoubleRangeParameter&>(model->parameterRef(index));
    p.values().clear();
    rebuildList(p);
    model->notifyParameterChange(index);
  }
  );
  layout2->addLayout(sublayout);

  layout->addLayout(layout2);

  layout->addStretch();

  return layout;
}
