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
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, name, parameter, defaultParameterSet)
{
}


QString IQDirectoryParameter::valueText() const
{
  const auto& p = dynamic_cast<const insight::DirectoryParameter&>(parameter());

  return QString::fromStdString( p.originalFilePath().string() );
}



QVBoxLayout* IQDirectoryParameter::populateEditControls(
        IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  const auto& p = dynamic_cast<const insight::DirectoryParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
  QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
  layout2->addWidget(promptLabel);
  auto *lineEdit=new QLineEdit(editControlsContainer);
//  connect(le_, &QLineEdit::destroyed, this, &PathParameterWrapper::onDestruction);
  lineEdit->setText(QString::fromStdString(p.originalFilePath().string()));
  layout2->addWidget(lineEdit);
  layout->addLayout(layout2);

  auto *dlgBtn_=new QPushButton("...", editControlsContainer);
  layout->addWidget(dlgBtn_);

  auto applyFunction = [=]()
  {
    auto&p = dynamic_cast<insight::DirectoryParameter&>(model->parameterRef(index));
    p.setOriginalFilePath( lineEdit->text().toStdString() );
    model->notifyParameterChange(index);
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
