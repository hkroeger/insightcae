
#include "base/parameterset.h"
#include "base/parameters.h"
#include "openfoam/openfoamdict.h"

#include "cellsetoption_selection.h"

#include "base/boost_include.h"
#include "base/exception.h"


namespace insight {

cellSetOption_Selection::cellSetOption_Selection(const Parameters& p)
  : p_(p)
{}


void cellSetOption_Selection::insertSelection(OFDictData::dict& d)
{
  if (const auto* all = boost::get<Parameters::selection_all_type>(&p_.selection))
    {
      d["selectionMode"]="all";
    }
  else if (const auto* cz = boost::get<Parameters::selection_cellZone_type>(&p_.selection))
    {
      d["selectionMode"]="cellZone";
      d["cellZone"]=cz->zoneName;
    }
  else if (const auto* cs = boost::get<Parameters::selection_cellSet_type>(&p_.selection))
    {
      d["selectionMode"]="cellSet";
      d["cellSet"]=cs->setName;
    }
  else
    {
      throw insight::Exception("Internal error: unhandled selection");
    }
}

} // namespace insight
