#include "insertedcaseelement.h"

InsertedCaseElement::InsertedCaseElement(QListWidget* parent, const std::string& type_name)
: QListWidgetItem(parent), type_name_(type_name)
{
    curp_ = insight::OpenFOAMCaseElement::defaultParameters(type_name);
    setText(type_name.c_str());
}


insight::OpenFOAMCaseElement* InsertedCaseElement::createElement(insight::OpenFOAMCase& c) const
{
    return insight::OpenFOAMCaseElement::lookup(type_name_, c, curp_);
}

void InsertedCaseElement::insertElement(insight::OpenFOAMCase& c) const
{
    c.insert(createElement(c));
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
