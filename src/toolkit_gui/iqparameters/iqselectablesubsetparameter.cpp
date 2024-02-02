#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QDebug>
#include <QFileDialog>

#include "iqselectablesubsetparameter.h"
#include "iqparametersetmodel.h"

#include "base/exception.h"
#include "base/tools.h"

#include "base/rapidxml.h"
#include "rapidxml/rapidxml_print.hpp"

defineType(IQSelectableSubsetParameter);
addToFactoryTable(IQParameter, IQSelectableSubsetParameter);

IQSelectableSubsetParameter::IQSelectableSubsetParameter
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


QString IQSelectableSubsetParameter::valueText() const
{
  const auto& p = dynamic_cast<const insight::SelectableSubsetParameter&>(parameter());

  return QString::fromStdString( p.selection() );
}



QVBoxLayout* IQSelectableSubsetParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  const auto&p = dynamic_cast<const insight::SelectableSubsetParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
  layout2->addWidget(new QLabel("Selection:", editControlsContainer));
  auto* selBox=new QComboBox(editControlsContainer);
  for ( auto& pair: p.items() )
  {
    selBox->addItem( QString::fromStdString(pair.first) );
  }
  selBox->setCurrentIndex(
        selBox->findText(
          QString::fromStdString(p.selection())
          )
        );

  layout2->addWidget(selBox);
  layout->addLayout(layout2);


  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);




//  auto* iqp = static_cast<IQParameter*>(index.internalPointer());
//  auto mp = model->pathFromIndex(index);

  connect(apply, &QPushButton::clicked, this, [this,selBox]()
  {
//    auto rindex = model->indexFromPath(mp);
//    Q_ASSERT(rindex.isValid());

    auto& param = dynamic_cast<insight::SelectableSubsetParameter&>(this->parameterRef());

    // change data, notify model
    param.setSelection(selBox->currentText().toStdString());
//    model->notifyParameterChange(rindex, true);
  }
  );

  return layout;
}



void IQSelectableSubsetParameter::populateContextMenu(QMenu *cm)
{
  auto *saveAction = new QAction("Save to file...");
  cm->addAction(saveAction);
  auto *loadAction = new QAction("Load from file...");
  cm->addAction(loadAction);

  QObject::connect(saveAction, &QAction::triggered, this,
                   [this]()
                   {
                       using namespace rapidxml;

                       auto fn = QFileDialog::getSaveFileName(nullptr, "Save selectable subset contents",
                                             QString(), "(*.iss)");
                       if (!fn.isEmpty())
                       {
                           auto file = insight::ensureFileExtension(fn.toStdString(), "iss");

                           auto &iqp = dynamic_cast<const insight::SelectableSubsetParameter&>(
                               this->parameter() );

                           insight::CurrentExceptionContext ex("writing parameter set to file "+file.string());
                           std::ofstream f(file.c_str());

                           // prepare XML document
                           xml_document<> doc;
                           xml_node<>* decl = doc.allocate_node(node_declaration);
                           decl->append_attribute(doc.allocate_attribute("version", "1.0"));
                           decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
                           doc.append_node(decl);

                           xml_node<> *rootnode = doc.allocate_node(
                               node_element,
                               insight::SelectableSubsetParameter::typeName_());
                           doc.append_node(rootnode);

                           // store parameters
                           iqp.appendToNode(this->name().toStdString(), doc, *rootnode, "");

                           f << doc;
                           f << std::endl;
                           f << std::flush;
                           f.close();
                       }
                   }
                   );


  QObject::connect(loadAction, &QAction::triggered, this,
                   [this]()
                   {
                       using namespace rapidxml;

                       auto fn = QFileDialog::getOpenFileName(nullptr, "Load selectable subset contents",
                                             QString(), "(*.iss)");
                       if (!fn.isEmpty())
                       {
                           auto file = insight::ensureFileExtension(fn.toStdString(), "iss");

                           auto &iqp = dynamic_cast<insight::SelectableSubsetParameter&>(
                               this->parameterRef() );

                           insight::CurrentExceptionContext ex("reading parameter set from file "+file.string());
                           std::string contents;
                           insight::readFileIntoString(file, contents);

                           // prepare XML document
                           xml_document<> doc;
                           doc.parse<0>(&contents[0]);
                           xml_node<> *rootnode = doc.first_node(insight::SelectableSubsetParameter::typeName_());


                           // store parameters
                           iqp.readFromNode(this->name().toStdString(), *rootnode, "");

//                           model->notifyParameterChange(index, true);
                       }
                   }
                   );
}
