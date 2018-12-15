#include "patch.h"

using namespace insight;
using namespace boost;
using namespace rapidxml;


Patch::Patch(QListWidget*parent, const std::string& patch_name)
: QListWidgetItem(parent), patch_name_(patch_name)
{
    setText(patch_name_.c_str());
}


Patch::Patch(QListWidget*parent, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, boost::filesystem::path inputfilepath)
: QListWidgetItem(parent)
{
    patch_name_ = node.first_attribute ( "patchName" )->value();
    setText(patch_name_.c_str());
    bc_type_ = node.first_attribute ( "BCtype" )->value();
    if (bc_type_!="")
    {
        set_bc_type(bc_type_);
        curp_.readFromNode(doc, node, inputfilepath);
    }
}


void Patch::set_bc_type(const std::string& type_name)
{
    bc_type_=type_name;
    setText( (patch_name_+" ("+bc_type_+")").c_str() );
    curp_ = BoundaryCondition::defaultParameters(bc_type_);
}

bool Patch::insertElement(insight::OpenFOAMCase& c, insight::OFDictData::dict& boundaryDict) const
{
    if (bc_type_!="")
    {
        c.insert(insight::BoundaryCondition::lookup(bc_type_, c, patch_name_, boundaryDict, curp_));
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
    node.append_attribute ( doc.allocate_attribute ( "BCtype", bc_type_.c_str() ) );

    curp_.appendToNode ( doc, node, inputfilepath.parent_path() );
}



DefaultPatch::DefaultPatch(QListWidget* parent)
: Patch(parent, "[Unassigned Patches]")
{
}

DefaultPatch::DefaultPatch(QListWidget*parent, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, boost::filesystem::path inputfilepath)
: Patch(parent, doc, node, inputfilepath)
{
}

bool DefaultPatch::insertElement ( insight::OpenFOAMCase& ofc, insight::OFDictData::dict& boundaryDict ) const
{
  if ( bc_type_!="" )
    {
      ofc.addRemainingBCs ( bc_type_, boundaryDict, curp_ );
      return true;
    }
  else
    {
      return false;
    }
}



