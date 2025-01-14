#ifndef INSERTEDCASEELEMENT_H
#define INSERTEDCASEELEMENT_H

#include <QObject>
#include <QListWidget>
#include <QListWidgetItem>

#include <string>

#ifndef Q_MOC_RUN
#include "openfoam/openfoamcase.h"
#include "openfoam/caseelements/boundarycondition.h"
#endif


#include "caseelementdata.h"



class InsertedCaseElement
: public CaseElementData
{
protected:
virtual
    insight::CADParameterSetModelVisualizer::VisualizerFunctions::Function
    getVisualizerFactoryFunction() override;

public:
    InsertedCaseElement(
        const std::string& type_name,
        insight::MultiCADParameterSetVisualizer::SubVisualizerList& mvl,
        MultivisualizationGenerator* visGen,
        QObject* parent=nullptr );

    insight::OpenFOAMCaseElement* createElement(insight::OpenFOAMCase& c) const;
    void insertElement(insight::OpenFOAMCase& ofc) const;

};



#endif // INSERTEDCASEELEMENT_H
