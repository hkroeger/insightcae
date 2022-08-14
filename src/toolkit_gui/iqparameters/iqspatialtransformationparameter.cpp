
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>

#include "iqspatialtransformationparameter.h"
#include "iqparametersetmodel.h"
#include "base/table.h"




defineType(IQSpatialTransformationParameter);
addToFactoryTable(IQParameter, IQSpatialTransformationParameter);




IQSpatialTransformationParameter::IQSpatialTransformationParameter
(
    QObject* parent,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, name, parameter, defaultParameterSet)
{
}




QString IQSpatialTransformationParameter::valueText() const
{
  const auto& p = dynamic_cast<const insight::SpatialTransformationParameter&>(parameter());

  if (p().isIdentityTransform())
      return QString("identity");
  else
      return QString("(transform)");
}





QVBoxLayout* IQSpatialTransformationParameter::populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer)
{
  const auto& p = dynamic_cast<const insight::SpatialTransformationParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer);

  QLineEdit *translateLE, *rpyLE, *scaleLE;

  {
      QHBoxLayout *layout2=new QHBoxLayout(editControlsContainer);
      QLabel *translateLabel = new QLabel("Translation:", editControlsContainer);
      layout2->addWidget(translateLabel);
      translateLE = new QLineEdit(editControlsContainer);
      translateLE->setText(mat2Str(p().translate()));
      layout2->addWidget(translateLE);
      layout->addLayout(layout2);
  }

  {
      QHBoxLayout *layout2=new QHBoxLayout(editControlsContainer);
      QLabel *rpyLabel = new QLabel("Roll Pitch Yaw:", editControlsContainer);
      layout2->addWidget(rpyLabel);
      rpyLE = new QLineEdit(editControlsContainer);
      rpyLE->setText(mat2Str(p().rollPitchYaw()));
      layout2->addWidget(rpyLE);
      layout->addLayout(layout2);
  }

  {
      QHBoxLayout *layout2=new QHBoxLayout(editControlsContainer);
      QLabel *scaleLabel = new QLabel("Scale factor:", editControlsContainer);
      layout2->addWidget(scaleLabel);
      scaleLE = new QLineEdit(editControlsContainer);
      scaleLE->setText(QString::number(p().scale()));
      layout2->addWidget(scaleLE);
      layout->addLayout(layout2);
  }

//  auto *dlgBtn_=new QPushButton("...", editControlsContainer);
//  layout2->addWidget(dlgBtn_);
//  layout->addLayout(layout2);

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);

  layout->addStretch();

  auto applyFunction = [=]()
  {
    auto&p = dynamic_cast<insight::SpatialTransformationParameter&>(model->parameterRef(index));

    arma::mat m;

    insight::stringToValue(translateLE->text().toStdString(), m);
    p().setTranslation(m);

    insight::stringToValue(rpyLE->text().toStdString(), m);
    p().setRollPitchYaw(m);

    bool ok;
    p().setScale(rpyLE->text().toDouble(&ok));
    insight::assertion(ok, "invalid input for scale factor!");

    model->notifyParameterChange(index);
  };

  connect(translateLE, &QLineEdit::returnPressed, applyFunction);
  connect(rpyLE, &QLineEdit::returnPressed, applyFunction);
  connect(scaleLE, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);

//  connect(dlgBtn_, &QPushButton::clicked, dlgBtn_,
//          [translateLE, editControlsContainer, applyFunction]()
//          {
//              QString fn = QFileDialog::getOpenFileName(
//                    editControlsContainer,
//                    "Select file",
//                    QString(),
//                    "CSV file (*.csv)" );
//              if (!fn.isEmpty())
//              {
//                  std::ifstream f(fn.toStdString());
//                  insight::Table tab(f);
//                  insight::assertion(
//                              tab.nCols()==2,
//                              str(boost::format("A table with 2 columns was expected! Got: %dx%d matrix.") % tab.nRows() % tab.nCols()) );
//                  translateLE->setText(mat2Str(tab.xy(0, 1)));
//                  applyFunction();
//              }
//          }
//  );

  return layout;
}
