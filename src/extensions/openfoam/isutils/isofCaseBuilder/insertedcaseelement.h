#ifndef INSERTEDCASEELEMENT_H
#define INSERTEDCASEELEMENT_H

#include <QListWidget>
#include <QListWidgetItem>

#include <string>

#ifndef Q_MOC_RUN
#include "openfoam/openfoamcase.h"
#include "openfoam/caseelements/boundarycondition.h"
#endif

class ParameterSetDisplay;

class CaseElementData
: public QListWidgetItem
{

protected:
  std::string type_name_;
  insight::ParameterSet curp_, defp_;

  ParameterSetDisplay* disp_;
  insight::ParameterSet_VisualizerPtr viz_;

public:
  CaseElementData(QListWidget*, const std::string& type_name, ParameterSetDisplay* d);
  ~CaseElementData();

  inline insight::ParameterSet& parameters() { return curp_; }
  inline const insight::ParameterSet& defaultParameters() { return defp_; }
  inline const insight::ParameterSet& parameters() const { return curp_; }

  insight::ParameterSet_VisualizerPtr visualizer();
  void updateVisualization();
};



class InsertedCaseElement
: public CaseElementData
{

public:
    InsertedCaseElement(QListWidget*, const std::string& type_name, ParameterSetDisplay* d);

    inline const std::string& type_name() const { return type_name_; }

    insight::OpenFOAMCaseElement* createElement(insight::OpenFOAMCase& c) const;
    void insertElement(insight::OpenFOAMCase& ofc) const;

};



#endif // INSERTEDCASEELEMENT_H
