#include "patch.h"

#include "openfoam/caseelements/boundarycondition.h"

#include "iqparametersetmodel.h"

using namespace insight;
using namespace boost;
using namespace rapidxml;

insight::CADParameterSetModelVisualizer::VisualizerFunctions::Function
Patch::getVisualizerFactoryFunction()
{
    if (insight::CADParameterSetModelVisualizer
        ::visualizerForOpenFOAMCaseElement().count(type_name_))
    {
        return insight::CADParameterSetModelVisualizer
            ::visualizerForOpenFOAMCaseElement().lookup(type_name_);
    }
    else
        return nullptr;
}

Patch::Patch(
    const std::string& patch_name,
    insight::MultiCADParameterSetVisualizer::SubVisualizerList& mvl,
    MultivisualizationGenerator* visGen,
    QObject* parent)
    : CaseElementData("", mvl, visGen, new IQParameterSetModel(ParameterSet::create()), parent),
  patch_name_(patch_name)
{
//  updateText();
}


Patch::Patch(const rapidxml::xml_node<>& node,
             boost::filesystem::path inputfilepath,
             insight::MultiCADParameterSetVisualizer::SubVisualizerList& mvl,
             MultivisualizationGenerator* visGen,
             QObject* parent
             )
: CaseElementData("", mvl, visGen, new IQParameterSetModel(ParameterSet::create()), parent)
{
  auto patchnameattr=node.first_attribute ( "patchName" );
  insight::assertion(patchnameattr, "Patch name attribute missing!");
  patch_name_ = patchnameattr->value();

//  updateText();

  auto typenameattr=node.first_attribute ( "BCtype" );
  insight::assertion(typenameattr, "Patch type attribute missing!");
  type_name_ = typenameattr->value();

  if (type_name_!="")
  {
      set_bc_type(type_name_);

      auto np = parameterSetModel()->getParameterSet().cloneParameterSet();
      np->readFromNode(std::string(), node, inputfilepath);

      parameterSetModel()->resetParameterValues(*np);
  }
}



//void Patch::updateText()
//{
//  if (type_name_.empty())
//    setText( QString::fromStdString(patch_name_) );
//  else
//    setText( QString::fromStdString(patch_name_+" ("+type_name_+")") );
//}

void Patch::set_bc_type(const std::string& type_name)
{
    type_name_=type_name;
    curp_->resetParameters(
        BoundaryCondition::defaultParametersFor(type_name_) );
    Q_EMIT visualizationUpdateRequired();
}

void Patch::set_patch_name(const QString& newname)
{
  patch_name_=newname.toStdString();
//  updateText();
}

bool Patch::insertElement(insight::OpenFOAMCase& c, insight::OFDictData::dict& boundaryDict) const
{
    if (type_name_!="")
    {
        c.insert(insight::BoundaryCondition::lookup(type_name_, c, patch_name_, boundaryDict, curp_->getParameterSet()));
        return true;
    }
    else
    {
        return false;
    }
}


void Patch::appendToNode ( rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, boost::filesystem::path inputfilepath )
{
//     xml_node<> *elemnode = doc.allocate_node ( node_element, "OpenFOAMCaseElement" );
    node.append_attribute ( doc.allocate_attribute ( "patchName", patch_name_.c_str() ) );
    node.append_attribute ( doc.allocate_attribute ( "BCtype", type_name_.c_str() ) );

    curp_->getParameterSet().appendToNode ( std::string(), doc, node, inputfilepath.parent_path() );
}


const QString DefaultPatch::defaultPatchName = "[Unassigned Patches]";

DefaultPatch::DefaultPatch(
    insight::MultiCADParameterSetVisualizer::SubVisualizerList& mvl,
    MultivisualizationGenerator* visGen,
    QObject* parent)
  : Patch(defaultPatchName.toStdString(), mvl, visGen, parent)
{}

DefaultPatch::DefaultPatch(
    const rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath,
    insight::MultiCADParameterSetVisualizer::SubVisualizerList& mvl,
    MultivisualizationGenerator* visGen,
    QObject* parent )
: Patch(node, inputfilepath, mvl, visGen, parent)
{}

bool DefaultPatch::insertElement ( insight::OpenFOAMCase& ofc, insight::OFDictData::dict& boundaryDict ) const
{
  if ( type_name_!="" )
    {
      ofc.addRemainingBCs ( type_name_, boundaryDict, curp_->getParameterSet() );
      return true;
    }
  else
    {
      return false;
    }
}



