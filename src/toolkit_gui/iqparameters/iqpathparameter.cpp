#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>
#include <QDesktopServices>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "iqpathparameter.h"
#include "iqparametersetmodel.h"
#include "qtextensions.h"
#include "base/externalprograms.h"

defineType(IQPathParameter);
addToFactoryTable(IQParameter, IQPathParameter);

IQPathParameter::IQPathParameter
(
    QObject* parent,
    IQParameterSetModel* psmodel,
    insight::Parameter* parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQSpecializedParameter<insight::PathParameter>(
          parent, psmodel, parameter, defaultParameterSet )
{
}


QString IQPathParameter::valueText() const
{
  return QString::fromStdString(
        parameter().originalFilePath().filename().string() );
}



QVBoxLayout* IQPathParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
  QHBoxLayout *layout3=new QHBoxLayout;
  QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
  layout2->addWidget(promptLabel);
  auto *lineEdit = new QLineEdit(editControlsContainer);
//  connect(le_, &QLineEdit::destroyed, this, &PathParameterWrapper::onDestruction);
  lineEdit->setText(QString::fromStdString(
      parameter().originalFilePath().string()));
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
    parameterRef().setOriginalFilePath( lineEdit->text().toStdString() );
  };

  connect(lineEdit, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);



  connect(lineEdit, &QLineEdit::textChanged, [=]()
  {
    lineEdit->setToolTip
    (
      QString("(Evaluates to \"%1\"")
              .arg(QString::fromStdString(
                boost::filesystem::absolute(
                      lineEdit->text().toStdString()).string() ))
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
      QProcess *sp = new QProcess(model());
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
    boost::filesystem::path orgfn(
          parameter().originalFilePath() );

    if (auto fn = getFileName(
          editControlsContainer,
          "Please select export path",
          GetFileMode::Save,
          {
            { boost::trim_left_copy_if(orgfn.extension().string(), boost::is_any_of(".")),
              "File" }
          }
          ) )
    {
      parameter().copyTo( fn.asString() );
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
