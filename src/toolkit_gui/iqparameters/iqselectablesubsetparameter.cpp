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
#include "qtextensions.h"



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

          if (auto fn = getFileName(
              nullptr, "Save selectable subset contents",
              GetFileMode::Save,
              {{ "iss", "Selectable subset contents" }} ) )
          {
              insight::CurrentExceptionContext ex(3, "writing parameter set to file "+fn.asString());
              std::ofstream f(fn.asString());

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

          if (auto fn = getFileName(
              nullptr, "Load selectable subset contents",
              GetFileMode::Open,
              {{ "iss", "Selectable subset contents" }}) )
          {
              auto file = insight::ensureFileExtension(fn, "iss");

              insight::XMLDocument doc(file);
              xml_node<> *rootnode = doc.first_node(insight::SelectableSubsetParameter::typeName_());


              // store parameters
              parameterRef().readFromNode(this->name().toStdString(), *rootnode, "");
          }
      }
      );
}


