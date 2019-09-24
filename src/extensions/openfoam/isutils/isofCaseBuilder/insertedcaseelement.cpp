#include "insertedcaseelement.h"
#include "parametereditorwidget.h"

CaseElementData::CaseElementData(QListWidget* parent, const std::string& type_name, ParameterSetDisplay* d)
  : QListWidgetItem(parent), disp_(d), type_name_(type_name)
{
  setText(type_name.c_str());
}

CaseElementData::~CaseElementData()
{
  if (disp_ && viz_)
  {
    disp_->deregisterVisualizer( std::dynamic_pointer_cast<insight::CAD_ParameterSet_Visualizer>(viz_) );
  }
}

insight::ParameterSet_VisualizerPtr CaseElementData::visualizer()
{
  return viz_;
}

void CaseElementData::updateVisualization()
{
  if (viz_)
  {
    viz_->update(curp_);
  }
}

InsertedCaseElement::InsertedCaseElement(QListWidget* parent, const std::string& type_name, ParameterSetDisplay* d)
: CaseElementData(parent, type_name, d)
{
    curp_ = insight::OpenFOAMCaseElement::defaultParameters(type_name);
    defp_ = curp_;
    if (type_name_!="")
    {
      try {
        viz_ = insight::OpenFOAMCaseElement::visualizer(type_name_);
        disp_->registerVisualizer( std::dynamic_pointer_cast<insight::CAD_ParameterSet_Visualizer>(viz_) );
      }
      catch (...)
      { /* skip */ }
    }
}



insight::OpenFOAMCaseElement* InsertedCaseElement::createElement(insight::OpenFOAMCase& c) const
{
    return insight::OpenFOAMCaseElement::lookup(type_name_, c, curp_);
}

void InsertedCaseElement::insertElement(insight::OpenFOAMCase& c) const
{
    c.insert(createElement(c));
}
