#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QIntValidator>

#include "iqintparameter.h"
#include "iqparametersetmodel.h"
#include "qtextensions.h"


defineType(IQIntParameter);
addToFactoryTable(IQParameter, IQIntParameter);

IQIntParameter::IQIntParameter
(
    QObject* parent,
    IQHierarchicalDataModel* hdmodel,
    insight::hierarchicalData::Element* element
)
  : IQSpecializedParameter<insight::IntParameter>(
          parent, hdmodel, element)
{}






QVBoxLayout* IQIntParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
  QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
  layout2->addWidget(promptLabel);
  auto* lineEdit=new QLineEdit(editControlsContainer);
  lineEdit->setText(QString::number( parameter()() ));
  lineEdit->setValidator(new QIntValidator());
  layout2->addWidget(lineEdit);
  layout->addLayout(layout2);

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);


  auto applyFunction = [=]()
  {
      parameterRef().set(lineEdit->text().toInt());
  };

  connect(lineEdit, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);

  // handle external value change
  ::disconnectAtEOL(
      layout,
      parameterRef().valueChanged.connect(
          [=]()
          {
              QSignalBlocker sb(lineEdit);
              lineEdit->setText(
                  QString::number(parameter()()));
          }
          )
      );

  return layout;
}
