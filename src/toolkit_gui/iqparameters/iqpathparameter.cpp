#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>
#include <QDesktopServices>

#include "iqpathparameter.h"
#include "iqparametersetmodel.h"

#include "base/externalprograms.h"

defineType(IQPathParameter);
addToFactoryTable(IQParameter, IQPathParameter);

IQPathParameter::IQPathParameter
(
    QObject* parent,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, name, parameter, defaultParameterSet)
{
}


QString IQPathParameter::valueText() const
{
  const auto& p = dynamic_cast<const insight::PathParameter&>(parameter());

  return QString::fromStdString( p.originalFilePath().filename().string() );
}



QVBoxLayout* IQPathParameter::populateEditControls(
        IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  const auto&p = dynamic_cast<const insight::PathParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
  QHBoxLayout *layout3=new QHBoxLayout;
  QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
  layout2->addWidget(promptLabel);
  auto *lineEdit = new QLineEdit(editControlsContainer);
//  connect(le_, &QLineEdit::destroyed, this, &PathParameterWrapper::onDestruction);
  lineEdit->setText(QString::fromStdString(p.originalFilePath().string()));
  layout2->addWidget(lineEdit);
  auto *dlgBtn_=new QPushButton("...", editControlsContainer);
  layout3->addWidget(dlgBtn_);
  auto *openBtn_=new QPushButton("Open", editControlsContainer);
  layout3->addWidget(openBtn_);
  auto *saveBtn=new QPushButton("Save...", editControlsContainer);
  layout3->addWidget(saveBtn);
  layout->addLayout(layout2);
  layout->addLayout(layout3);

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);


  auto applyFunction = [=]()
  {
    auto&p = dynamic_cast<insight::PathParameter&>(model->parameterRef(index));
    p.setOriginalFilePath( lineEdit->text().toStdString() );
    model->notifyParameterChange(index);
  };

  connect(lineEdit, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);



  connect(lineEdit, &QLineEdit::textChanged, [=]()
  {
    lineEdit->setToolTip
    (
      QString("(Evaluates to \"")+boost::filesystem::absolute(lineEdit->text().toStdString()).string().c_str()+"\")"
    );
  }
  );


  connect(dlgBtn_, &QPushButton::clicked, [=]()
  {
    QString fn = showSelectPathDialog(editControlsContainer, lineEdit->text());
    if (!fn.isEmpty())
    {
      lineEdit->setText(fn);
      applyFunction();
    }
  }
  );


  connect(openBtn_, &QPushButton::clicked, [=]()
  {
    //if ( !QDesktopServices::openUrl(QUrl("file://"+le_->text())) )
    boost::filesystem::path fp( lineEdit->text().toStdString() );
    std::string ext=fp.extension().string();
    boost::algorithm::to_lower(ext);

    QString program;
    if ( (ext==".stl")||(ext==".stlb") )
    {
      program=QString::fromStdString( insight::ExternalPrograms::path("paraview").string() );
    }
    else if ( (ext==".stp")||(ext==".step")||(ext==".igs")||(ext==".iges")||(ext==".iscad")||(ext==".brep") )
    {
      program=QString::fromStdString( insight::ExternalPrograms::path("iscad").string() );
    }

    if (!program.isEmpty())
    {
      QProcess *sp = new QProcess(model);
      sp->start(program, QStringList() << lineEdit->text() );

      if (!sp->waitForStarted())
      {
        QMessageBox::critical(editControlsContainer, "Could not open file", "Could not launch program: "+program);
      }
    }
    else
    {
      if (!QDesktopServices::openUrl(QUrl("file://"+lineEdit->text())))
      {
        QMessageBox::critical(editControlsContainer, "Could not open file", "Could not open the file using QDesktopServices!");
      }
    }
  }
  );


  connect(saveBtn, &QPushButton::clicked, [=]()
  {
    const auto&p = dynamic_cast<const insight::PathParameter&>(parameter());
    boost::filesystem::path orgfn( p.originalFilePath() );
    QString fn = QFileDialog::getSaveFileName(
          editControlsContainer,
          "Please select export path",
          QString(),
          QString::fromStdString("(*."+orgfn.extension().string()+")")
          );
    if (!fn.isEmpty())
    {
      p.copyTo( fn.toStdString() );
    }
  }
  );


  return layout;
}

QString IQPathParameter::showSelectPathDialog(QWidget* parent, const QString& startPath) const
{
  return QFileDialog::getOpenFileName(
      parent,
      "Select file",
      startPath);
}
