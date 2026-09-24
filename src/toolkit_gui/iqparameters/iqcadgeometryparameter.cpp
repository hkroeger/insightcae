#include "iqcadgeometryparameter.h"
#include "iqparametersetmodel.h"
#include "iqcadmodel3dviewer.h"
#include "iqcaditemmodel.h"

#include "qtextensions.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QInputDialog>
#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "base/externalprograms.h"


defineType(IQCADGeometryParameter);
addToFactoryTable(IQParameter, IQCADGeometryParameter);




IQCADGeometryParameter::IQCADGeometryParameter
(
    QObject* parent,
    IQHierarchicalDataModel* hdmodel,
    insight::hierarchicalData::Element* element
)
  : IQSpecializedParameter<insight::CADGeometryParameter>(
          parent, hdmodel, element )
{
}




QVBoxLayout* IQCADGeometryParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  const auto&p = dynamic_cast<const insight::CADGeometryParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);


  QHBoxLayout *layout2=new QHBoxLayout;
  // QLabel *promptLabel = new QLabel("Feature label:", editControlsContainer);
  // layout2->addWidget(promptLabel);
  // auto *leFeatureLabel = new QLineEdit(editControlsContainer);
  // leFeatureLabel->setText(QString::fromStdString(p.featureLabel()));
  // layout2->addWidget(leFeatureLabel);
  // layout->addLayout(layout2);

  // auto *teScript = new QTextEdit(editControlsContainer);
  // teScript->document()->setPlainText(QString::fromStdString(p.script()));
  // layout->addWidget(teScript);

  QHBoxLayout *layout3=new QHBoxLayout;
  QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
  layout2->addWidget(promptLabel);
  auto *lineEdit = new QLineEdit(editControlsContainer);
  //  connect(le_, &QLineEdit::destroyed, this, &PathParameterWrapper::onDestruction);
  if (auto fn=parameter().filePath())
  {
    lineEdit->setText(QString::fromStdString(
      fn->string()));
  }

  ::disconnectAtEOL(
      lineEdit,
      parameterRef().valueChanged.connect(
          [lineEdit,this](){
              DBG_SLOT(valueChanged);
              if (auto fn=parameter().filePath())
              {
                  lineEdit->setText(QString::fromStdString(
                      fn->string()));
              }
          }
          )
      );
  layout2->addWidget(lineEdit);
  layout->addLayout(layout2);

  auto *dlgBtn_=new QPushButton("...", editControlsContainer);
  layout3->addWidget(dlgBtn_);
  auto *openBtn_=new QPushButton("Open", editControlsContainer);
  layout3->addWidget(openBtn_);
  auto *saveBtn=new QPushButton("Save...", editControlsContainer);
  layout3->addWidget(saveBtn);
  layout->addLayout(layout3);

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);


  auto applyFunction = [=]()
  {
    // auto&p = dynamic_cast<insight::CADGeometryParameter&>(this->parameterRef());
//    p.setCADModel( viewer->cadmodel()->model() );
    //p.setScript( teScript->document()->toPlainText().toStdString() );
    this->parameterRef().setGeometryFile(lineEdit->text().toStdString());
//    model->notifyParameterChange(index);
  };

  // connect(leFeatureLabel, &QLineEdit::returnPressed, applyFunction);
  connect(lineEdit, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);



  // connect(leFeatureLabel, &QLineEdit::textChanged, [=]()
  // {
  //   leFeatureLabel->setToolTip
  //   (
  //     QString("(Evaluates to \"")+boost::filesystem::absolute(leFeatureLabel->text().toStdString()).string().c_str()+"\")"
  //   );
  // }
  // );


  connect(dlgBtn_, &QPushButton::clicked, dlgBtn_,
          [=]()
          {
              QString fn = QFileDialog::getOpenFileName(
          editControlsContainer,
          "Select file",
          lineEdit->text());
              if (!fn.isEmpty())
              {
                  parameterRef().setGeometryFile(fn.toStdString());
                  // lineEdit->setText(fn);
                  // applyFunction();
              }

            // bool ok=false;
            // QStringList items;
            // auto mf = viewer->cadmodel()->modelsteps();
            // std::transform(
            //           mf.begin(), mf.end(),
            //           std::back_inserter(items),
            //           [](const decltype(mf)::value_type& mfv)
            //           { return QString::fromStdString(mfv.first); }
            // );
            // auto itemLbl = QInputDialog::getItem(
            //           editControlsContainer,
            //           "Select feature",
            //           leFeatureLabel->text(),
            //           items, items.indexOf(leFeatureLabel->text()),
            //           false, &ok);
            // if (ok)
            // {
            //     auto symbolName = itemLbl.toStdString();
            //     auto selfeat = viewer->cadmodel()->model()->modelsteps().at(symbolName);
            //     leFeatureLabel->setText(itemLbl);
            //     teScript->setPlainText( QString::fromStdString(
            //         symbolName+": "+
            //         selfeat->generateScriptCommand()
            //         ) );
            //     applyFunction();
            // }
          }
  );


  connect(openBtn_, &QPushButton::clicked, openBtn_, [=]()
  {
    auto afp = parameter().accessibleFilePath();
    if (!afp)
    {
      QMessageBox::critical(editControlsContainer, "Could not open file", "The geometry is not stored in a file!");
      return;
    }
    QString fn = QString::fromStdString(afp->string());

    std::string ext=afp->extension().string();
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
      sp->start(program, QStringList() << fn );

      if (!sp->waitForStarted())
      {
        QMessageBox::critical(editControlsContainer, "Could not open file", "Could not launch program: "+program);
      }
    }
    else
    {
      if (!QDesktopServices::openUrl(QUrl::fromLocalFile(fn)))
      {
        QMessageBox::critical(editControlsContainer, "Could not open file", "Could not open the file using QDesktopServices!");
      }
    }
  }
  );


  connect(saveBtn, &QPushButton::clicked, saveBtn, [=]()
  {
    auto afp = parameter().accessibleFilePath();
    if (!afp)
    {
      QMessageBox::critical(editControlsContainer, "Could not save file", "The geometry is not stored in a file!");
      return;
    }

    if (auto fn = getFileName(
          editControlsContainer,
          "Please select export path",
          GetFileMode::Save,
          {
            { boost::trim_left_copy_if(afp->extension().string(), boost::is_any_of(".")),
              "File" }
          }
          ) )
    {
      boost::filesystem::copy_file(
          *afp, fn.asString(),
          boost::filesystem::copy_option::overwrite_if_exists );
    }
  }
  );


  return layout;
}
