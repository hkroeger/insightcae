#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QDebug>
#include <QFileDialog>

#include "iqselectablesubsetparameter.h"
#include "base/factory.h"
#include "iqparametersetmodel.h"

#include "base/exception.h"
#include "base/tools.h"

#include "base/rapidxml.h"
#include "rapidxml/rapidxml_print.hpp"



defineTemplateType(IQSelectionParameterBase<insight::SelectableSubsetParameter>);
defineType(IQSelectableSubsetParameter);
addToFactoryTable(IQParameter, IQSelectableSubsetParameter);




addFunctionToStaticFunctionTable(
    IQParameterGridViewDelegateEditorWidget, IQSelectableSubsetParameter,
    createDelegate,
    [](QObject* parent) { return new IQSelectionDelegate(parent); }
    );




IQSelectableSubsetParameter::IQSelectableSubsetParameter
(
    QObject* parent,
    IQParameterSetModel* psmodel,
    insight::Parameter* parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQSelectionParameterBase<insight::SelectableSubsetParameter>(
          parent, psmodel, parameter, defaultParameterSet)
{}




void IQSelectableSubsetParameter::populateContextMenu(QMenu *cm)
{
  auto *saveAction = new QAction("Save to file...");
  cm->addAction(saveAction);
  auto *loadAction = new QAction("Load from file...");
  cm->addAction(loadAction);

  QObject::connect(
      saveAction, &QAction::triggered, this,
      [this]()
      {
          using namespace rapidxml;

          auto fn = QFileDialog::getSaveFileName(
              nullptr, "Save selectable subset contents",
              QString(), "(*.iss)");

          if (!fn.isEmpty())
          {
              auto file = insight::ensureFileExtension(fn.toStdString(), "iss");

              insight::CurrentExceptionContext ex(3, "writing parameter set to file "+file.string());
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
              parameter().appendToNode(this->name().toStdString(), doc, *rootnode, "");

              f << doc;
              f << std::endl;
              f << std::flush;
              f.close();
          }
      }
      );


  QObject::connect(
      loadAction, &QAction::triggered, this,
      [this]()
      {
          using namespace rapidxml;

          auto fn = QFileDialog::getOpenFileName(
              nullptr, "Load selectable subset contents",
              QString(), "(*.iss)");

          if (!fn.isEmpty())
          {
              auto file = insight::ensureFileExtension(fn.toStdString(), "iss");

              insight::CurrentExceptionContext ex("reading parameter set from file "+file.string());
              std::string contents;
              insight::readFileIntoString(file, contents);

              // prepare XML document
              xml_document<> doc;
              doc.parse<0>(&contents[0]);
              xml_node<> *rootnode = doc.first_node(insight::SelectableSubsetParameter::typeName_());


              // store parameters
              parameterRef().readFromNode(this->name().toStdString(), *rootnode, "");
          }
      }
      );
}


