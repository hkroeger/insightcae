#ifndef NUMERICALWINDTUNNEL_GUI_H
#define NUMERICALWINDTUNNEL_GUI_H

#include "numericalwindtunnel.h"
#include "cadparametersetvisualizer.h"

namespace insight
{

class NumericalWindtunnel_ParameterSet_Visualizer
    : public AnalysisVisualizer<NumericalWindtunnel>
{
public:
    using AnalysisVisualizer<NumericalWindtunnel>::AnalysisVisualizer;

    void recreateVisualizationElements() override;
};


}

#endif // NUMERICALWINDTUNNEL_GUI_H
