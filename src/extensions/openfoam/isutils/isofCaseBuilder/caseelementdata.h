#ifndef CASEELEMENTDATA_H
#define CASEELEMENTDATA_H

#include "cadparametersetvisualizer.h"

#include <QPointer>


namespace insight {
class MultiCADParameterSetVisualizer;
}

class MultivisualizationGenerator
{
public:
    virtual void rebuildVisualization() =0;
};



class CaseElementData
    : public QObject
{
    Q_OBJECT

protected:
    std::string type_name_;
    IQParameterSetModel *curp_;

    insight::MultiCADParameterSetVisualizer::SubVisualizerList& mv_;
    MultivisualizationGenerator* mvGen_;

    virtual
        insight::CADParameterSetModelVisualizer::VisualizerFunctions::Function
        getVisualizerFactoryFunction() =0;

public:
    CaseElementData(
        const std::string& type_name,
        insight::MultiCADParameterSetVisualizer::SubVisualizerList& mvl,
        MultivisualizationGenerator* visGen,
        IQParameterSetModel *psm,
        QObject* parent = nullptr );

    ~CaseElementData();

    IQParameterSetModel* parameterSetModel() const;

    inline const std::string& type_name() const { return type_name_; }

    insight::MultiCADParameterSetVisualizer::SubVisualizerList& multiVisualizer() const;

Q_SIGNALS:
    void visualizationUpdateRequired();
};
;

#endif // CASEELEMENTDATA_H
