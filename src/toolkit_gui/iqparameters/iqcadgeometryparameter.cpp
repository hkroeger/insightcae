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
    IQParameterSetModel* psmodel,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, psmodel, name, parameter, defaultParameterSet)
{
}




QString IQCADGeometryParameter::valueText() const
{
  const auto& p = dynamic_cast<const insight::CADGeometryParameter&>(parameter());

  return QString::fromStdString( p.featureLabel() );
}




QVBoxLayout* IQCADGeometryParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  const auto&p = dynamic_cast<const insight::CADGeometryParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);


  QHBoxLayout *layout2=new QHBoxLayout;
  QLabel *promptLabel = new QLabel("Feature label:", editControlsContainer);
  layout2->addWidget(promptLabel);
  auto *leFeatureLabel = new QLineEdit(editControlsContainer);
  leFeatureLabel->setText(QString::fromStdString(p.featureLabel()));
  layout2->addWidget(leFeatureLabel);
  layout->addLayout(layout2);

  auto *teScript = new QTextEdit(editControlsContainer);
  teScript->document()->setPlainText(QString::fromStdString(p.script()));
  layout->addWidget(teScript);

  QHBoxLayout *layout3=new QHBoxLayout;
  auto *dlgBtn_=new QPushButton("...", editControlsContainer);
  layout3->addWidget(dlgBtn_);
  layout->addLayout(layout3);

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);


  auto applyFunction = [=]()
  {
    auto&p = dynamic_cast<insight::CADGeometryParameter&>(this->parameterRef());
//    p.setCADModel( viewer->cadmodel()->model() );
    p.setFeatureLabel( leFeatureLabel->text().toStdString() );
    p.setScript( teScript->document()->toPlainText().toStdString() );
//    model->notifyParameterChange(index);
  };

  connect(leFeatureLabel, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);



  connect(leFeatureLabel, &QLineEdit::textChanged, [=]()
  {
    leFeatureLabel->setToolTip
    (
      QString("(Evaluates to \"")+boost::filesystem::absolute(leFeatureLabel->text().toStdString()).string().c_str()+"\")"
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
                      leFeatureLabel->text(),
                      items, items.indexOf(leFeatureLabel->text()),
                      false, &ok);
            if (ok)
            {
                auto symbolName = itemLbl.toStdString();
                auto selfeat = viewer->cadmodel()->model()->modelsteps().at(symbolName);
                leFeatureLabel->setText(itemLbl);
                teScript->setPlainText( QString::fromStdString(
                    symbolName+": "+
                    selfeat->generateScriptCommand()
                    ) );
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
