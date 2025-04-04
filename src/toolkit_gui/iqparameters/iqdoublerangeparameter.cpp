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
    IQParameterSetModel* psmodel,
    insight::Parameter* parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQSpecializedParameter<insight::DoubleRangeParameter>(
          parent, psmodel, parameter, defaultParameterSet )
{
}


QString IQDoubleRangeParameter::valueText() const
{
  return QString("%1 values")
        .arg( parameter().values().size() );
}



QVBoxLayout* IQDoubleRangeParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{

  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

  QLabel *promptLabel = new QLabel("Selection:", editControlsContainer);
  layout->addWidget(promptLabel);

  QHBoxLayout *layout2=new QHBoxLayout;
  auto *listBox = new QListWidget(editControlsContainer);
//  connect(lBox_, &QListWidget::destroyed, this, &DoubleRangeParameterWrapper::onDestruction);
  layout2->addWidget(listBox);

  auto rebuildList = [=](const insight::DoubleRangeParameter& cp)
  {
    int crow=listBox->currentRow();
    listBox->clear();
    for (auto i=cp.values().begin();
         i!=cp.values().end(); i++)
    {
      listBox->addItem( QString::number(*i) );
    }
    listBox->setCurrentRow(crow);
  };
  rebuildList(parameter());

  QVBoxLayout *sublayout=new QVBoxLayout(editControlsContainer);

  QPushButton *addbtn=new QPushButton("Add...", editControlsContainer);
  sublayout->addWidget(addbtn);
  connect(addbtn, &QPushButton::clicked, [=]()
  {
    bool ok;
    double v=QInputDialog::getDouble(editControlsContainer, "Add Range", "Please specify value:", 0., -2147483647,  2147483647, 9, &ok);
    if (ok)
    {
      parameterRef().insertValue(v);
      rebuildList(parameter());
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
      QStringList il=res.split(" ", Qt::SkipEmptyParts);
      double x0=il[0].toDouble();
      double x1=il[1].toDouble();
      int num=il[2].toInt();
      for (int i=0; i<num; i++)
      {
        double x=x0+(x1-x0)*double(i)/double(num-1);
        parameterRef().insertValue(x);
        rebuildList(parameter());
      }
    }
  }
  );

  QPushButton *clearbtn=new QPushButton("Clear", editControlsContainer);
  sublayout->addWidget(clearbtn);
  connect(clearbtn, &QPushButton::clicked, [=]()
  {
    parameterRef().clear();
    rebuildList(parameter());
  }
  );
  layout2->addLayout(sublayout);

  layout->addLayout(layout2);


  return layout;
}
