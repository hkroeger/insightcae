#ifndef ANALYSISPARAMETERPROPOSITIONS_H
#define ANALYSISPARAMETERPROPOSITIONS_H

#include "base/parameterset.h"

namespace insight
{

class AnalysisParameterPropositions
{
public:
    typedef std::function<ParameterSet(
        const std::string&,
        const ParameterSet&)> propositionGeneratorFunction;

private:
    std::map<std::string, std::vector<propositionGeneratorFunction> > propositionGeneratorFunctions_;

    AnalysisParameterPropositions();

    static std::unique_ptr<AnalysisParameterPropositions> instance;

public:

    /**
     * @brief getCombinedPropositionsForParameter
     * combines propositions from static functions and python source
     * @param analysisName
     * @param parameterPath
     * @param currentParameterValues
     * @return
     */
    static ParameterSet getCombinedPropositionsForParameter(
        const std::string& analysisName,
        const std::string& parameterPath,
        const ParameterSet& currentParameterValues );
};

}

#endif // ANALYSISPARAMETERPROPOSITIONS_H
