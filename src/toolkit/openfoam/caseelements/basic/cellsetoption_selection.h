#ifndef INSIGHT_CELLSETOPTION_SELECTION_H
#define INSIGHT_CELLSETOPTION_SELECTION_H

#include <string>
#include "base/boost_include.h"

#include "cellsetoption_selection__cellSetOption_Selection__Parameters_headers.h"

namespace insight {

namespace OFDictData { class dict; }

class cellSetOption_Selection
{
public:
#include "cellsetoption_selection__cellSetOption_Selection__Parameters.h"
/*
PARAMETERSET>>> cellSetOption_Selection Parameters

selection = selectablesubset {{
 all set {}
 cellZone set {
  zoneName = string "zone" "Name of the cell zone"
 }
 cellSet set {
  setName = string "set" "Name of the cell set"
 }
}} all "Selection of cells"

<<<PARAMETERSET
*/
protected:
  Parameters p_;

public:
  cellSetOption_Selection(const insight::cellSetOption_Selection::Parameters& p);

  void insertSelection(OFDictData::dict& d);
};


} // namespace insight

#endif // INSIGHT_CELLSETOPTION_SELECTION_H
