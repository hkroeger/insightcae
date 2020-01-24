#include "patch.h"

using namespace insight;
using namespace boost;
using namespace rapidxml;


Patch::Patch(QListWidget*parent, const std::string& patch_name, ParameterSetDisplay* d)
: CaseElementData(parent, "", d),
  patch_name_(patch_name)
{
  updateText();
}


Patch::Patch(QListWidget*parent,
             rapidxml::xml_document<>& doc,
             rapidxml::xml_node<>& node,
             boost::filesystem::path inputfilepath,
             ParameterSetDisplay* d
             )
: CaseElementData(parent, "", d)
{
  auto patchnameattr=node.first_attribute ( "patchName" );
  insight::assertion(patchnameattr, "Patch name attribute missing!");
  patch_name_ = patchnameattr->value();

  updateText();

  auto typenameattr=node.first_attribute ( "BCtype" );
  insight::assertion(typenameattr, "Patch type attribute missing!");
  type_name_ = typenameattr->value();

  if (type_name_!="")
  {
      set_bc_type(type_name_);
      curp_.readFromNode(doc, node, inputfilepath);
  }
}

void Patch::updateText()
{
  if (type_name_.empty())
    setText( QString::fromStdString(patch_name_) );
  else
    setText( QString::fromStdString(patch_name_+" ("+type_name_+")") );
}

void Patch::set_bc_type(const std::string& type_name)
{
    type_name_=type_name;
    updateText();
    curp_ = BoundaryCondition::defaultParameters(type_name_);
    defp_ = curp_;
    if (type_name_!="")
    {
      try {
        viz_ = insight::BoundaryCondition::visualizer(type_name_);
      }
      catch (...) { /* skip */ }
    }
}

void Patch::set_patch_name(const QString& newname)
{
  patch_name_=newname.toStdString();
  updateText();
}

bool Patch::insertElement(insight::OpenFOAMCase& c, insight::OFDictData::dict& boundaryDict) const
{
    if (type_name_!="")
    {
        c.insert(insight::BoundaryCondition::lookup(type_name_, c, patch_name_, boundaryDict, curp_));
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

    curp_.appendToNode ( doc, node, inputfilepath.parent_path() );
}


const QString DefaultPatch::defaultPatchName = "[Unassigned Patches]";

DefaultPatch::DefaultPatch(QListWidget* parent, ParameterSetDisplay* d)
  : Patch(parent, defaultPatchName.toStdString(), d)
{
}

DefaultPatch::DefaultPatch(QListWidget*parent, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, boost::filesystem::path inputfilepath, ParameterSetDisplay* d)
: Patch(parent, doc, node, inputfilepath, d)
{
}

bool DefaultPatch::insertElement ( insight::OpenFOAMCase& ofc, insight::OFDictData::dict& boundaryDict ) const
{
  if ( type_name_!="" )
    {
      ofc.addRemainingBCs ( type_name_, boundaryDict, curp_ );
      return true;
    }
  else
    {
      return false;
    }
}



