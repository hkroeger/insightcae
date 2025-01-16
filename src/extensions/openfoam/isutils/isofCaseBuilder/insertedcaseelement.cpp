#include "insertedcaseelement.h"
#include "parametereditorwidget.h"


insight::CADParameterSetModelVisualizer::VisualizerFunctions::Function
InsertedCaseElement::getVisualizerFactoryFunction()
{
    if (insight::CADParameterSetModelVisualizer
        ::visualizerForOpenFOAMCaseElement_table().count(type_name_))
    {
        return insight::CADParameterSetModelVisualizer
            ::visualizerForOpenFOAMCaseElement_table().lookup(type_name_);
    }
    else
        return nullptr;
}



InsertedCaseElement::InsertedCaseElement(
    const std::string& type_name,
    insight::MultiCADParameterSetVisualizer::SubVisualizerList& mvl,
    MultivisualizationGenerator* visGen,
    QObject *parent
    )
    : CaseElementData(
          type_name, mvl, visGen,
          new IQParameterSetModel(
              insight::OpenFOAMCaseElement::defaultParametersFor(type_name)
            ),
          parent)
{}




insight::OpenFOAMCaseElement* InsertedCaseElement::createElement(insight::OpenFOAMCase& c) const
{
    return insight::OpenFOAMCaseElement::lookup(type_name_, c, curp_->getParameterSet());
}




void InsertedCaseElement::insertElement(insight::OpenFOAMCase& c) const
{
    c.insert(createElement(c));
}
