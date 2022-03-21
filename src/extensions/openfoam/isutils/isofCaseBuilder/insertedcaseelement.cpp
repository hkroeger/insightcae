#include "insertedcaseelement.h"
#include "parametereditorwidget.h"




CaseElementData::CaseElementData(
    const std::string& type_name,
    insight::Multi_CAD_ParameterSet_Visualizer* mv,
    QObject *parent )

  : QObject(parent),
    type_name_(type_name),
    mv_(mv)
{}




CaseElementData::~CaseElementData()
{
  if (mv_ && viz_)
  {
    mv_->unregisterVisualizer( std::dynamic_pointer_cast<insight::CAD_ParameterSet_Visualizer>(viz_).get() );
  }
}




const insight::ParameterSet CaseElementData::defaultParameters() const
{
  return insight::OpenFOAMCaseElement::defaultParameters(type_name_);
}




insight::ParameterSet_VisualizerPtr CaseElementData::visualizer()
{
  return viz_;
}


insight::Multi_CAD_ParameterSet_Visualizer* CaseElementData::multiVisualizer() const
{
  return mv_;
}

void CaseElementData::updateVisualization()
{
  if (viz_)
  {
    viz_->update(curp_);
  }
}




InsertedCaseElement::InsertedCaseElement(
    const std::string& type_name,
    insight::Multi_CAD_ParameterSet_Visualizer* mv,
    QObject *parent
    )
: CaseElementData(type_name, mv, parent)
{
    curp_ = defaultParameters();
    if (type_name_!="")
    {
      try {
        viz_ = insight::OpenFOAMCaseElement::visualizer(type_name_);
        mv_->registerVisualizer( std::dynamic_pointer_cast<insight::CAD_ParameterSet_Visualizer>(viz_).get() );
      }
      catch (...)
      { /* skip, if there is no visualizer defined */ }
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
