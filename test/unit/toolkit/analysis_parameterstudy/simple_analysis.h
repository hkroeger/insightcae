#ifndef SIMPLE_ANALYSIS_H
#define SIMPLE_ANALYSIS_H

#include "base/analysis.h"
#include "base/parameterstudy.h"
#include "simple_analysis__SimpleAnalysis__Parameters_headers.h"

namespace insight
{


class SimpleAnalysis
: public AnalysisWithParameters
{
public:
#include "simple_analysis__SimpleAnalysis__Parameters.h"

/*
PARAMETERSET>>> SimpleAnalysis Parameters
x = double 0 "x value"
<<<PARAMETERSET
*/

    addParameterMembers_ParameterClass(Parameters);

public:
    declareType("SimpleAnalysis");
    SimpleAnalysis(const std::shared_ptr<supplementedInputDataBase>& sp);
    ResultSetPtr operator()(ProgressDisplayer& p = consoleProgressDisplayer) override;

    static std::string category() { return "Test"; }
    static AnalysisDescription description() { return {"Test", ""}; }
};


}




#endif // SIMPLE_ANALYSIS_H
