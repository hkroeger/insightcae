#include "iqcadgeometryparameter.h"
#include "iqparametersetmodel.h"
#include "iqcadmodel3dviewer.h"
#include "iqcaditemmodel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QInputDialog>


defineType(IQCADGeometryParameter);
addToFactoryTable(IQParameter, IQCADGeometryParameter);




IQCADGeometryParameter::IQCADGeometryParameter
(
    QObject* parent,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, name, parameter, defaultParameterSet)
{
}




QString IQCADGeometryParameter::valueText() const
{
  const auto& p = dynamic_cast<const insight::CADGeometryParameter&>(parameter());

  return QString::fromStdString( p.featureLabel() );
}




QVBoxLayout* IQCADGeometryParameter::populateEditControls(
        IQParameterSetModel* model,
        const QModelIndex &index,
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  const auto&p = dynamic_cast<const insight::CADGeometryParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
  QHBoxLayout *layout3=new QHBoxLayout;
  QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
  layout2->addWidget(promptLabel);
  auto *lineEdit = new QLineEdit(editControlsContainer);
//  connect(le_, &QLineEdit::destroyed, this, &PathParameterWrapper::onDestruction);
  lineEdit->setText(QString::fromStdString(p.featureLabel()));
  layout2->addWidget(lineEdit);
  auto *dlgBtn_=new QPushButton("...", editControlsContainer);
  layout3->addWidget(dlgBtn_);
//  auto *openBtn_=new QPushButton("Open", editControlsContainer);
//  layout3->addWidget(openBtn_);
//  auto *saveBtn=new QPushButton("Save...", editControlsContainer);
//  layout3->addWidget(saveBtn);
  layout->addLayout(layout2);
  layout->addLayout(layout3);

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);


  auto applyFunction = [=]()
  {
    auto&p = dynamic_cast<insight::CADGeometryParameter&>(model->parameterRef(index));
    p.setCADModel( viewer->cadmodel()->model() );
    p.setFeatureLabel( lineEdit->text().toStdString() );
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


  connect(dlgBtn_, &QPushButton::clicked, dlgBtn_,
          [=]()
          {
            bool ok=false;
            QStringList items;
            auto mf = viewer->cadmodel()->modelsteps();
            std::transform(
                      mf.begin(), mf.end(),
                      std::back_inserter(items),
                      [](const decltype(mf)::value_type& mfv)
                      { return QString::fromStdString(mfv.first); }
            );
            auto itemLbl = QInputDialog::getItem(
                      editControlsContainer,
                      "Select feature",
                      lineEdit->text(),
                      items, items.indexOf(lineEdit->text()),
                      false, &ok);
            if (ok)
            {
              lineEdit->setText(itemLbl);
              applyFunction();
            }
          }
  );


//  connect(openBtn_, &QPushButton::clicked, [=]()
//  {
//    //if ( !QDesktopServices::openUrl(QUrl("file://"+le_->text())) )
//    boost::filesystem::path fp( lineEdit->text().toStdString() );
//    std::string ext=fp.extension().string();
//    boost::algorithm::to_lower(ext);

//    QString program;
//    if ( (ext==".stl")||(ext==".stlb") )
//    {
//      program=QString::fromStdString( insight::ExternalPrograms::path("paraview").string() );
//    }
//    else if ( (ext==".stp")||(ext==".step")||(ext==".igs")||(ext==".iges")||(ext==".iscad")||(ext==".brep") )
//    {
//      program=QString::fromStdString( insight::ExternalPrograms::path("iscad").string() );
//    }

//    if (!program.isEmpty())
//    {
//      QProcess *sp = new QProcess(model);
//      sp->start(program, QStringList() << lineEdit->text() );

//      if (!sp->waitForStarted())
//      {
//        QMessageBox::critical(editControlsContainer, "Could not open file", "Could not launch program: "+program);
//      }
//    }
//    else
//    {
//      if (!QDesktopServices::openUrl(QUrl("file://"+lineEdit->text())))
//      {
//        QMessageBox::critical(editControlsContainer, "Could not open file", "Could not open the file using QDesktopServices!");
//      }
//    }
//  }
//  );


//  connect(saveBtn, &QPushButton::clicked, [=]()
//  {
//    const auto&p = dynamic_cast<const insight::PathParameter&>(parameter());
//    boost::filesystem::path orgfn( p.originalFilePath() );
//    QString fn = QFileDialog::getSaveFileName(
//          editControlsContainer,
//          "Please select export path",
//          QString(),
//          QString::fromStdString("(*."+orgfn.extension().string()+")")
//          );
//    if (!fn.isEmpty())
//    {
//      p.copyTo( fn.toStdString() );
//    }
//  }
//  );


  return layout;
}
