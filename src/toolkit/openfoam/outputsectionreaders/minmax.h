#ifndef MINMAX_H
#define MINMAX_H

#include "openfoam/outputsectionreader.h"

namespace insight {
namespace OutputSectionReaders {



class MinMax : public OutputSectionReader
{
    std::string label_;
    std::map<std::string, double> min_, max_;

    MinMax(const std::string& label);

public:
    declareType("MinMaxReader");

    static std::unique_ptr<OutputSectionReader> createIfMatches(
        const std::string& line );

    bool parseNextLine(const std::string& line) override;
    void addProgressVariables(std::map<std::string, double>& progVars) const override;
};



} // namespace OutputSectionReaders
} // namespace insight

#endif // MINMAX_H
