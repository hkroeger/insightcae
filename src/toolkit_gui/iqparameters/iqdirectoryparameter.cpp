#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>

#include "iqdirectoryparameter.h"
#include "iqparametersetmodel.h"

defineType(IQDirectoryParameter);
addToFactoryTable(IQParameter, IQDirectoryParameter);

IQDirectoryParameter::IQDirectoryParameter
(
    QObject* parent,
    IQParameterSetModel* psmodel,
    insight::Parameter* parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQSpecializedParameter<insight::DirectoryParameter>(
          parent, psmodel, parameter, defaultParameterSet)
{
}


QString IQDirectoryParameter::valueText() const
{
  return QString::fromStdString(
        parameter().originalFilePath().string() );
}



QVBoxLayout* IQDirectoryParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{

  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
  QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
  layout2->addWidget(promptLabel);
  auto *lineEdit=new QLineEdit(editControlsContainer);
  lineEdit->setText(QString::fromStdString(
      parameter().originalFilePath().string()));
  layout2->addWidget(lineEdit);
  layout->addLayout(layout2);

  auto *dlgBtn_=new QPushButton("...", editControlsContainer);
  layout->addWidget(dlgBtn_);

  auto applyFunction = [=]()
  {
    parameterRef().setOriginalFilePath( lineEdit->text().toStdString() );
  };

  connect(lineEdit, &QLineEdit::returnPressed, applyFunction);

  connect(dlgBtn_, &QPushButton::clicked, [=]()
  {
    QString fn = QFileDialog::getExistingDirectory(
          editControlsContainer,
          "Select directory",
          lineEdit->text());
    if (!fn.isEmpty())
    {
      lineEdit->setText(fn);
      applyFunction();
    }
  }
  );

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  connect(apply, &QPushButton::pressed, applyFunction);
  layout->addWidget(apply);


  return layout;
}
