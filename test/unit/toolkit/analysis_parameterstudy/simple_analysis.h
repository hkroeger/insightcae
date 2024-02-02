#ifndef SIMPLE_ANALYSIS_H
#define SIMPLE_ANALYSIS_H

#include "base/analysis.h"
#include "base/parameterstudy.h"
#include "simple_analysis__SimpleAnalysis__Parameters_headers.h"

namespace insight
{


class SimpleAnalysis
: public Analysis
{
public:
#include "simple_analysis__SimpleAnalysis__Parameters.h"

/*
PARAMETERSET>>> SimpleAnalysis Parameters
x = double 0 "x value"
<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
    declareType("SimpleAnalysis");
    SimpleAnalysis(const ParameterSet& ps, const boost::filesystem::path& exepath, ProgressDisplayer& pd);
    static std::string category() { return "Test"; }
    ParameterSet parameters() const override { return p_; }
    ResultSetPtr operator()(ProgressDisplayer& p = consoleProgressDisplayer) override;
};


}




#endif // SIMPLE_ANALYSIS_H
