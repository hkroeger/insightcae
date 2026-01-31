#ifndef OUTPUTSECTIONREADER_H
#define OUTPUTSECTIONREADER_H

#include <string.h>

#include "base/factory.h"
#include "base/boost_include.h"

namespace insight {

class OutputSectionReader;

typedef std::shared_ptr<OutputSectionReader> OutputSectionReaderPtr;

class OutputSectionReader
{
public:
    declareStaticFunctionTable2(
        Readers, createIfMatches,
        std::unique_ptr<OutputSectionReader>,
        const std::string& );

    declareType ( "OutputSectionReader" );

    virtual bool parseNextLine(const std::string& line);
    virtual void addProgressVariables(std::map<std::string, double>& progVars) const =0;
};

} // namespace insight

#endif // OUTPUTSECTIONREADER_H
