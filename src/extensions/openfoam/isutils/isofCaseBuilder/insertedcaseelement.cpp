#include "insertedcaseelement.h"

InsertedCaseElement::InsertedCaseElement(QListWidget* parent, const std::string& type_name)
: QListWidgetItem(parent), type_name_(type_name)
{
    curp_ = insight::OpenFOAMCaseElement::defaultParameters(type_name);
    setText(type_name.c_str());
}


void InsertedCaseElement::insertElement(insight::OpenFOAMCase& c) const
{
    c.insert(insight::OpenFOAMCaseElement::lookup(type_name_, c, curp_));
}

bool InsertedCaseElement::hasVisualization() const
{
  try {
      insight::OpenFOAMCaseElement::visualizer(type_name());
      return true;
  }
  catch (insight::Exception e)
  {
    return false;
  }
}
